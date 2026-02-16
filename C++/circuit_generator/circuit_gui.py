import tkinter as tk
from tkinter import messagebox, font
import subprocess
import json
import os

# Configuration
GENERATOR_PATH = "./circuit_generator"
SVG_PATH = "circuit.svg"
PNG_PATH = "circuit.png"

# Modern Color Palette
COLORS = {
    'bg': '#1a1a2e',           # Dark blue-gray background
    'card': '#16213e',         # Card background
    'accent': '#0f4c75',       # Accent blue
    'highlight': '#3282b8',    # Bright blue
    'success': '#4ecca3',      # Green for correct
    'error': '#ff6b6b',        # Red for incorrect
    'text': '#eee',            # Light text
    'text_dim': '#a8a8a8',     # Dimmed text
    'button': '#3282b8',       # Button color
    'button_hover': '#4a9fd8'  # Button hover
}

class ModernButton(tk.Button):
    """Custom styled button with hover effect"""
    def __init__(self, parent, **kwargs):
        # Extract custom params
        hover_color = kwargs.pop('hover_color', COLORS['button_hover'])
        normal_color = kwargs.pop('bg', COLORS['button'])
        
        super().__init__(parent, **kwargs)
        self.config(
            bg=normal_color,
            fg=COLORS['text'],
            font=('Segoe UI', 11, 'bold'),
            bd=0,
            padx=20,
            pady=10,
            cursor='hand2',
            activebackground=hover_color,
            activeforeground=COLORS['text']
        )
        
        self._normal_color = normal_color
        self._hover_color = hover_color
        
        self.bind('<Enter>', lambda e: self.config(bg=self._hover_color))
        self.bind('<Leave>', lambda e: self.config(bg=self._normal_color))

class CircuitApp:
    def __init__(self, root):
        self.root = root
        self.root.title("⚡ Tutor Circuiti Elettrici")
        self.root.geometry("800x500")
        self.root.config(bg=COLORS['bg'])
        
        # Fonts - JetBrains Mono like IntelliJ
        self.font_family = 'JetBrains Mono'
        self.title_font = font.Font(family=self.font_family, size=18, weight='bold')
        self.subtitle_font = font.Font(family=self.font_family, size=9)
        self.label_font = font.Font(family=self.font_family, size=9)
        self.input_font = font.Font(family=self.font_family, size=9)
        
        self.solution = {}
        self.question_text = ""
        self.exercise_type = "DC"  # DC or AC
        self.exercise_history = []  # Stack of (circuit_data, solution, exercise_type)
        self.current_index = -1  # Current position in history
        
        # Main container - zero padding
        main_container = tk.Frame(root, bg=COLORS['bg'])
        main_container.pack(fill=tk.BOTH, expand=True, padx=3, pady=3)
        
        # Header
        self.create_header(main_container)
        
        # Circuit Image Card
        self.create_image_card(main_container)
        
        # Question Card
        self.create_question_card(main_container)
        
        # Input Card
        self.create_input_card(main_container)
        
        # Status bar
        self.status_label = tk.Label(
            main_container,
            text="Pronto",
            bg=COLORS['bg'],
            fg=COLORS['text_dim'],
            font=self.label_font,
            anchor='w'
        )
        self.status_label.pack(fill=tk.X, pady=(2, 0))
        
        # Initial generation
        self.generate_circuit()

    def create_header(self, parent):
        header = tk.Frame(parent, bg=COLORS['bg'])
        header.pack(fill=tk.X, pady=(0, 3))
        
        # Title - smaller
        title = tk.Label(
            header,
            text="⚡ Tutor Circuiti Elettrici",
            font=self.title_font,
            bg=COLORS['bg'],
            fg=COLORS['highlight']
        )
        title.pack(side=tk.LEFT)
        
        # Exercise Type Selector
        type_frame = tk.Frame(header, bg=COLORS['bg'])
        type_frame.pack(side=tk.RIGHT)
        
        tk.Label(type_frame, text="Tipo:", bg=COLORS['bg'], fg=COLORS['text'], font=self.label_font).pack(side=tk.LEFT, padx=(0, 5))
        
        self.type_var = tk.StringVar(value="DC")
        for ex_type_val, ex_type_text in [("DC", "Transitorio DC"), ("AC", "Regime AC")]:
            rb = tk.Radiobutton(
                type_frame,
                text=ex_type_text,
                variable=self.type_var,
                value=ex_type_val,
                bg=COLORS['bg'],
                fg=COLORS['text'],
                selectcolor=COLORS['accent'],
                font=self.label_font,
                command=self.on_type_change
            )
            rb.pack(side=tk.LEFT, padx=2)
        
        # Navigation Buttons
        nav_frame = tk.Frame(header, bg=COLORS['bg'])
        nav_frame.pack(side=tk.RIGHT, padx=(0, 10))
        
        self.btn_prev = ModernButton(
            nav_frame,
            text="< Prec",
            command=self.previous_exercise,
            bg=COLORS['button']
        )
        self.btn_prev.pack(side=tk.LEFT, padx=(0, 5))
        
        self.btn_next = ModernButton(
            nav_frame,
            text="Succ >",
            command=self.next_exercise,
            bg=COLORS['button']
        )
        self.btn_next.pack(side=tk.LEFT)

    def create_image_card(self, parent):
        card = tk.Frame(parent, bg=COLORS['card'], relief=tk.FLAT, bd=0)
        card.pack(fill=tk.X, pady=(0, 3))
        
        # Card title - smaller header
        title_bar = tk.Frame(card, bg=COLORS['accent'], height=25)
        title_bar.pack(fill=tk.X)
        title_bar.pack_propagate(False)
        
        title_label = tk.Label(
            title_bar,
            text="Schema Circuito",
            font=(self.font_family, 9, 'bold'),
            bg=COLORS['accent'],
            fg=COLORS['text']
        )
        title_label.pack(side=tk.LEFT, padx=12, pady=6)
        
        # Image container - zero padding
        img_container = tk.Frame(card, bg=COLORS['card'])
        img_container.pack(fill=tk.X, padx=3, pady=3)
        
        self.img_label = tk.Label(
            img_container,
            text="Generazione circuito...",
            bg=COLORS['card'],
            fg=COLORS['text_dim'],
            font=self.subtitle_font
        )
        self.img_label.pack()

    def create_question_card(self, parent):
        card = tk.Frame(parent, bg=COLORS['card'], relief=tk.FLAT, bd=0)
        card.pack(fill=tk.X, pady=(0, 8))
        
        # Card title - smaller
        title_bar = tk.Frame(card, bg=COLORS['accent'], height=25)
        title_bar.pack(fill=tk.X)
        title_bar.pack_propagate(False)
        
        title_label = tk.Label(
            title_bar,
            text="Richiesta",
            font=('Segoe UI', 10, 'bold'),
            bg=COLORS['accent'],
            fg=COLORS['text']
        )
        title_label.pack(side=tk.LEFT, padx=12, pady=4)
        
        # Question text - less padding
        question_container = tk.Frame(card, bg=COLORS['card'])
        question_container.pack(fill=tk.X, padx=12, pady=8)
        
        self.question_label = tk.Label(
            question_container,
            text="",
            bg=COLORS['card'],
            fg=COLORS['text'],
            font=(self.font_family, 9),
            wraplength=750,
            justify=tk.LEFT,
            anchor='w'
        )
        self.question_label.pack(fill=tk.X)

    def create_input_card(self, parent):
        card = tk.Frame(parent, bg=COLORS['card'], relief=tk.FLAT, bd=0)
        card.pack(fill=tk.X)
        
        # Card title - smaller
        title_bar = tk.Frame(card, bg=COLORS['accent'], height=28)
        title_bar.pack(fill=tk.X)
        title_bar.pack_propagate(False)
        
        title_label = tk.Label(
            title_bar,
            text="Le Tue Risposte",
            font=('Segoe UI', 10, 'bold'),
            bg=COLORS['accent'],
            fg=COLORS['text']
        )
        title_label.pack(side=tk.LEFT, padx=12, pady=4)
        
        # Inputs container - zero padding
        self.inputs_container = tk.Frame(card, bg=COLORS['card'], pady=3, padx=8)
        self.inputs_container.pack(fill=tk.X)
        
        self.entries = {}
        self.feedback_labels = {}
        
        # Create initial DC inputs
        self.create_dc_inputs()

    def create_input_row(self, parent, key, label_text, row):
        row_frame = tk.Frame(parent, bg=COLORS['card'])
        row_frame.grid(row=row, column=0, columnspan=3, sticky='ew', pady=2)
        parent.grid_columnconfigure(1, weight=1)
        
        # Label
        lbl = tk.Label(
            row_frame,
            text=label_text,
            bg=COLORS['card'],
            fg=COLORS['text'],
            font=self.label_font,
            width=22,
            anchor='w'
        )
        lbl.grid(row=0, column=0, sticky='w', padx=(0, 8))
        
        # Entry with rounded appearance
        ent = tk.Entry(
            row_frame,
            font=self.input_font,
            bg=COLORS['bg'],
            fg=COLORS['text'],
            insertbackground=COLORS['text'],
            bd=0,
            relief=tk.FLAT,
            width=18
        )
        ent.grid(row=0, column=1, sticky='ew', padx=(0, 8), ipady=6)
        ent.bind('<Return>', lambda e, k=key: self.check_answer(k))
        self.entries[key] = ent
        
        # Check button - smaller
        btn = ModernButton(
            row_frame,
            text="Check",
            command=lambda k=key: self.check_answer(k),
            bg=COLORS['highlight']
        )
        btn.config(padx=15, pady=6)  # Smaller button
        btn.grid(row=0, column=2, padx=(0, 8))
        
        # Feedback label
        fb = tk.Label(
            row_frame,
            text="",
            bg=COLORS['card'],
            fg=COLORS['text'],
            font=(self.font_family, 8),
            anchor='w'
        )
        fb.grid(row=0, column=3, sticky='w', padx=(8, 0))
        self.feedback_labels[key] = fb
    
    def on_type_change(self):
        """Called when user changes exercise type"""
        new_type = self.type_var.get()
        if new_type != self.exercise_type:
            # Clear history when changing type
            self.exercise_history = []
            self.current_index = -1
            self.exercise_type = new_type
            self.generate_circuit()
    
    def previous_exercise(self):
        """Go to previous exercise in history"""
        if self.current_index > 0:
            self.current_index -= 1
            self.load_exercise_from_history(self.current_index)
            self.update_nav_buttons()
    
    def next_exercise(self):
        """Go to next exercise (generate new if at end)"""
        if self.current_index < len(self.exercise_history) - 1:
            # Navigate forward in history
            self.current_index += 1
            self.load_exercise_from_history(self.current_index)
        else:
            # Generate new exercise
            self.generate_circuit()
        self.update_nav_buttons()
    
    def update_nav_buttons(self):
        """Enable/disable navigation buttons based on position"""
        if self.current_index <= 0:
            self.btn_prev.config(state=tk.DISABLED)
        else:
            self.btn_prev.config(state=tk.NORMAL)
    
    def load_exercise_from_history(self, index):
        """Load a previously generated exercise from history"""
        if 0 <= index < len(self.exercise_history):
            data = self.exercise_history[index]
            self.solution = data['solution']
            self.exercise_type = data['type']
            
            # Reload SVG/PNG (they should still exist)
            self.load_image()
            
            # Recreate UI
            ex_type = self.solution.get("exercise_type", "DC")
            if ex_type == "AC":
                resistor_names = list(self.solution.get("avg_power", {}).keys())
                self.create_ac_inputs(resistor_names)
                freq = self.solution.get("frequency", 0)
                omega = self.solution.get("omega", 0)
                waveform = self.solution.get("source_waveform", "sin")
                amplitude = self.solution.get("source_amplitude", 1)
                self.question_text = f"Circuito AC con i_s(t) = {amplitude}*{waveform}({int(omega)}t) A. Calcola i valori richiesti."
            else:
                self.create_dc_inputs()
                var = self.solution.get("variable", "x")
                self.question_text = f"L'interruttore cambia stato a t=0. Calcola la risposta transitoria {var}(t)."
            
            self.question_label.config(text=self.question_text)
            self.status_label.config(text=f"Esercizio {index + 1} di {len(self.exercise_history)}", fg=COLORS['text_dim'])
    
    def clear_inputs(self):
        """Remove all current input rows"""
        for widget in self.inputs_container.winfo_children():
            widget.destroy()
        self.entries.clear()
        self.feedback_labels.clear()
    
    def create_dc_inputs(self):
        """Create input fields for DC transient mode"""
        self.clear_inputs()
        labels = [
            ('initial', 'Valore Iniziale (t=0⁻)'),
            ('final', 'Valore Finale (t=∞)'),
            ('tau', 'Costante di Tempo (τ)')
        ]
        for i, (key, label_text) in enumerate(labels):
            self.create_input_row(self.inputs_container, key, label_text, i)
    
    def create_ac_inputs(self, resistor_names):
        """Create input fields for AC mode with dynamic resistor power inputs"""
        self.clear_inputs()
        row = 0
        
        # Power inputs for each resistor
        for rname in resistor_names:
            key = f"power_{rname}"
            label = f"Potenza Media {rname} (W)"
            self.create_input_row(self.inputs_container, key, label, row)
            row += 1
        
        # Power factor if needed
        if 'power_factor' in self.solution:
            key = "power_factor"
            label = "Fattore di Potenza"
            self.create_input_row(self.inputs_container, key, label, row)

    def generate_circuit(self):
        self.status_label.config(text="Generazione nuovo circuito...", fg=COLORS['text_dim'])
        self.root.update()
        
        # Clear previous feedback
        for k in self.feedback_labels:
            self.feedback_labels[k].config(text="", fg=COLORS['text'])
        for k in self.entries:
            self.entries[k].delete(0, tk.END)
        
        self.question_label.config(text="")
            
        try:
            # 1. Run C++ Generator with exercise type
            exercise_arg = self.exercise_type
            result = subprocess.run([GENERATOR_PATH, "headless", exercise_arg], capture_output=True, text=True)
            if result.returncode != 0:
                messagebox.showerror("Errore", "Generatore fallito:\n" + result.stderr)
                self.status_label.config(text="Errore generazione circuito", fg=COLORS['error'])
                return
                
            # Parse JSON
            output_str = result.stdout.strip()
            start_idx = output_str.find("{")
            end_idx = output_str.rfind("}") + 1
            if start_idx != -1 and end_idx != -1:
                json_str = output_str[start_idx:end_idx]
                self.solution = json.loads(json_str)
            else:
                raise ValueError("Nessun JSON trovato nell'output")
            
            # 2. Convert SVG to PNG
            subprocess.run(["rsvg-convert", SVG_PATH, "-o", PNG_PATH], check=True)
            
            # 3. Load Image
            self.load_image()
            
            # 4. Update UI based on exercise type
            ex_type = self.solution.get("exercise_type", "DC")
            
            if ex_type == "AC":
                # AC Mode
                freq = self.solution.get("frequency", 0)
                omega = self.solution.get("omega", 0)
                waveform = self.solution.get("source_waveform", "sin")
                amplitude = self.solution.get("source_amplitude", 1)
                
                # Create input fields for resistor powers
                resistor_names = list(self.solution.get("avg_power", {}).keys())
                self.create_ac_inputs(resistor_names)
                
                # Question text with concrete amplitude
                self.question_text = f"Circuito AC con i_s(t) = {amplitude}*{waveform}({int(omega)}t) A. Calcola i valori richiesti."
                
            else:
                # DC Mode
                self.create_dc_inputs()
                var = self.solution.get("variable", "x")
                self.question_text = f"L'interruttore cambia stato a t=0. Calcola la risposta transitoria {var}(t)."
            
            self.question_label.config(text=self.question_text)
            
            # 5. Add to history
            # Remove any exercises after current index (if user went back and generated new)
            if self.current_index < len(self.exercise_history) - 1:
                self.exercise_history = self.exercise_history[:self.current_index + 1]
            
            # Add new exercise
            self.exercise_history.append({
                'solution': self.solution.copy(),
                'type': self.exercise_type,
                'question': self.question_text
            })
            self.current_index = len(self.exercise_history) - 1
            
            # Update navigation
            self.update_nav_buttons()
            
            self.status_label.config(text=f"Esercizio {self.current_index + 1} di {len(self.exercise_history)} caricato.", fg=COLORS['success'])
            
        except Exception as e:
            messagebox.showerror("Errore", str(e))
            self.status_label.config(text=f"Errore: {str(e)}", fg=COLORS['error'])

    def load_image(self):
        try:
            self.photo = tk.PhotoImage(file=PNG_PATH)
            self.img_label.config(image=self.photo, text="", bg=COLORS['card'])
        except Exception as e:
            self.img_label.config(text=f"Failed to load image: {e}", fg=COLORS['error'])

    def check_answer(self, key):
        user_text = self.entries[key].get().strip()
        if not user_text:
            return
            
        try:
            val = float(user_text)
            
            # Determine correct value based on exercise type and key
            ex_type = self.solution.get("exercise_type", "DC")
            correct_val = None
            
            if ex_type == "AC":
                if key.startswith("power_"):
                    # Extract resistor name from key (e.g., "power_R2" -> "R2")
                    rname = key[6:]  # Remove "power_" prefix
                    correct_val = self.solution.get("avg_power", {}).get(rname)
                elif key == "power_factor":
                    correct_val = self.solution.get("power_factor")
            else:
                # DC mode
                correct_val = self.solution.get(key)
            
            if correct_val is None:
                self.feedback_labels[key].config(text="✖ Soluzione non disponibile", fg=COLORS['error'])
                return
            
            # Calculate error
            diff = abs(val - correct_val)
            denom = abs(correct_val) if abs(correct_val) > 1e-9 else 1.0
            error = diff / denom if abs(correct_val) > 1e-9 else diff
            
            if error < 0.05:
                self.feedback_labels[key].config(text="✓ Corretto!", fg=COLORS['success'])
                # Flash effect
                self.entries[key].config(bg=COLORS['success'])
                self.root.after(200, lambda: self.entries[key].config(bg=COLORS['bg']))
            else:
                self.feedback_labels[key].config(
                    text=f"✗ Errato (Atteso: {correct_val:.4g})",
                    fg=COLORS['error']
                )
                # Flash effect
                self.entries[key].config(bg=COLORS['error'])
                self.root.after(200, lambda: self.entries[key].config(bg=COLORS['bg']))
                
        except ValueError:
            self.feedback_labels[key].config(text="✖ Numero invalido", fg=COLORS['error'])

if __name__ == "__main__":
    root = tk.Tk()
    app = CircuitApp(root)
    root.mainloop()
