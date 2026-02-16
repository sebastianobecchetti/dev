// Circuit Tutor Web App - Frontend Logic

class CircuitApp {
    constructor() {
        this.exerciseType = 'DC';
        this.history = [];
        this.currentIndex = -1;
        this.currentSolution = null;

        this.init();
    }

    init() {
        // Get DOM elements
        this.circuitImg = document.getElementById('circuit-img');
        this.loading = document.getElementById('loading');
        this.questionText = document.getElementById('question-text');
        this.inputsContainer = document.getElementById('inputs-container');
        this.statusText = document.getElementById('status-text');
        this.exerciseCounter = document.getElementById('exercise-counter');
        this.btnPrev = document.getElementById('btn-prev');
        this.btnNext = document.getElementById('btn-next');

        // Theme elements
        this.themeBtn = document.getElementById('theme-btn');
        this.themeMenu = document.getElementById('theme-menu');

        // Load saved theme
        this.loadTheme();

        // Setup event listeners
        this.setupEventListeners();

        // Generate first circuit
        this.generateCircuit();
    }

    setupEventListeners() {
        // Theme toggle
        this.themeBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            this.themeMenu.classList.toggle('hidden');
        });

        // Theme options
        document.querySelectorAll('.theme-option').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const theme = e.target.dataset.theme;
                this.setTheme(theme);
                this.themeMenu.classList.add('hidden');
            });
        });

        // Close theme menu when clicking outside
        document.addEventListener('click', (e) => {
            if (!this.themeMenu.classList.contains('hidden') &&
                !this.themeMenu.contains(e.target) &&
                !this.themeBtn.contains(e.target)) {
                this.themeMenu.classList.add('hidden');
            }
        });

        // Type toggle buttons
        document.querySelectorAll('.toggle-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const type = e.target.dataset.type;
                if (type !== this.exerciseType) {
                    this.exerciseType = type;
                    this.history = [];
                    this.currentIndex = -1;

                    // Update active button
                    document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));
                    e.target.classList.add('active');

                    this.generateCircuit();
                }
            });
        });

        // Navigation buttons
        this.btnPrev.addEventListener('click', () => this.previousExercise());
        this.btnNext.addEventListener('click', () => this.nextExercise());
    }

    async generateCircuit() {
        this.setStatus('Generazione nuovo circuito...', false);
        this.showLoading(true);

        try {
            const response = await fetch(`/api/generate/${this.exerciseType}`);
            const data = await response.json();

            if (!data.success) {
                throw new Error(data.error || 'Generation failed');
            }

            this.currentSolution = data.solution;

            // Load circuit image
            this.circuitImg.src = '/api/circuit.png?' + new Date().getTime();
            this.circuitImg.onload = () => {
                this.circuitImg.classList.add('loaded');
                this.showLoading(false);
            };

            // Update UI
            this.updateQuestion();
            this.createInputFields();

            // Add to history
            if (this.currentIndex < this.history.length - 1) {
                this.history = this.history.slice(0, this.currentIndex + 1);
            }
            this.history.push({
                solution: this.currentSolution,
                type: this.exerciseType
            });
            this.currentIndex = this.history.length - 1;

            this.updateNavButtons();
            this.setStatus(`Esercizio ${this.currentIndex + 1} di ${this.history.length} caricato.`, true);
            this.updateCounter();

        } catch (error) {
            console.error('Error:', error);
            this.setStatus(`Errore: ${error.message}`, false);
            this.showLoading(false);
        }
    }

    updateQuestion() {
        const exType = this.currentSolution.exercise_type;

        if (exType === 'AC') {
            const omega = this.currentSolution.omega;
            const waveform = this.currentSolution.source_waveform;
            const amplitude = this.currentSolution.source_amplitude;
            this.questionText.textContent =
                `Circuito AC con i_s(t) = ${amplitude}*${waveform}(${omega}t) A. Calcola i valori richiesti.`;
        } else {
            const variable = this.currentSolution.variable || 'x';
            this.questionText.textContent =
                `L'interruttore cambia stato a t=0. Calcola la risposta transitoria ${variable}(t).`;
        }
    }

    createInputFields() {
        this.inputsContainer.innerHTML = '';
        const exType = this.currentSolution.exercise_type;

        if (exType === 'AC') {
            // Power inputs for each resistor
            const avgPower = this.currentSolution.avg_power || {};
            Object.keys(avgPower).forEach(resistor => {
                this.addInputRow(`power_${resistor}`, `Potenza Media ${resistor} (W)`);
            });

            // Power factor if available
            if ('power_factor' in this.currentSolution) {
                this.addInputRow('power_factor', 'Fattore di Potenza');
            }
        } else {
            // DC inputs
            this.addInputRow('initial', 'Valore Iniziale (t=0⁻)');
            this.addInputRow('final', 'Valore Finale (t=∞)');
            this.addInputRow('tau', 'Costante di Tempo (τ)');
        }
    }

    addInputRow(key, label) {
        const row = document.createElement('div');
        row.className = 'input-row';

        const labelEl = document.createElement('label');
        labelEl.className = 'input-label';
        labelEl.textContent = label;

        const input = document.createElement('input');
        input.type = 'text';
        input.className = 'input-field';
        input.dataset.key = key;
        input.placeholder = 'Inserisci risposta...';
        input.addEventListener('input', () => this.checkAnswer(key, input));

        const feedback = document.createElement('span');
        feedback.className = 'feedback';
        feedback.dataset.key = key;

        row.appendChild(labelEl);
        row.appendChild(input);
        row.appendChild(feedback);

        this.inputsContainer.appendChild(row);
    }

    async checkAnswer(key, inputEl) {
        const value = inputEl.value.trim();
        if (!value) {
            inputEl.classList.remove('correct', 'incorrect');
            this.updateFeedback(key, '');
            return;
        }

        try {
            const userVal = parseFloat(value);
            if (isNaN(userVal)) {
                this.updateFeedback(key, '✖ Numero invalido', 'incorrect');
                inputEl.classList.remove('correct');
                inputEl.classList.add('incorrect');
                return;
            }

            // Get correct value
            let correctVal;
            const exType = this.currentSolution.exercise_type;

            if (exType === 'AC') {
                if (key.startsWith('power_')) {
                    const resistor = key.substring(6);
                    correctVal = this.currentSolution.avg_power[resistor];
                } else if (key === 'power_factor') {
                    correctVal = this.currentSolution.power_factor;
                }
            } else {
                correctVal = this.currentSolution[key];
            }

            if (correctVal == null) {
                this.updateFeedback(key, '✖ Soluzione non disponibile', 'incorrect');
                return;
            }

            // Calculate error
            const diff = Math.abs(userVal - correctVal);
            const denom = Math.abs(correctVal) > 1e-9 ? Math.abs(correctVal) : 1.0;
            const error = Math.abs(correctVal) > 1e-9 ? diff / denom : diff;

            if (error < 0.05) {
                inputEl.classList.remove('incorrect');
                inputEl.classList.add('correct');
                this.updateFeedback(key, '✓ Corretto!', 'correct');
            } else {
                inputEl.classList.remove('correct');
                inputEl.classList.add('incorrect');
                this.updateFeedback(key, `✗ Errato (Atteso: ${correctVal.toPrecision(4)})`, 'incorrect');
            }

        } catch (error) {
            console.error('Check answer error:', error);
        }
    }

    updateFeedback(key, text, className = '') {
        const feedback = document.querySelector(`.feedback[data-key="${key}"]`);
        if (feedback) {
            feedback.textContent = text;
            feedback.className = 'feedback ' + className;
        }
    }

    previousExercise() {
        if (this.currentIndex > 0) {
            this.currentIndex--;
            this.loadExercise(this.currentIndex);
            this.updateNavButtons();
        }
    }

    nextExercise() {
        if (this.currentIndex < this.history.length - 1) {
            this.currentIndex++;
            this.loadExercise(this.currentIndex);
        } else {
            this.generateCircuit();
        }
        this.updateNavButtons();
    }

    loadExercise(index) {
        const exercise = this.history[index];
        this.currentSolution = exercise.solution;
        this.exerciseType = exercise.type;

        // Reload image
        this.circuitImg.src = '/api/circuit.png?' + new Date().getTime();
        this.circuitImg.classList.add('loaded');

        // Update UI
        this.updateQuestion();
        this.createInputFields();
        this.setStatus(`Esercizio ${index + 1} di ${this.history.length}`, true);
        this.updateCounter();
    }

    updateNavButtons() {
        this.btnPrev.disabled = this.currentIndex <= 0;
    }

    updateCounter() {
        if (this.history.length > 0) {
            this.exerciseCounter.textContent =
                `Esercizio ${this.currentIndex + 1} / ${this.history.length}`;
        }
    }

    setStatus(text, isSuccess) {
        this.statusText.textContent = text;
        this.statusText.style.color = isSuccess ? '#10b981' : '#a0aec0';
    }

    showLoading(show) {
        if (show) {
            this.loading.classList.remove('hidden');
            this.circuitImg.classList.remove('loaded');
        } else {
            this.loading.classList.add('hidden');
        }
    }

    // Theme Management
    loadTheme() {
        const savedTheme = localStorage.getItem('circuitAppTheme') || 'gruvbox';
        this.setTheme(savedTheme, false);
    }

    setTheme(theme, save = true) {
        document.body.setAttribute('data-theme', theme);

        // Update active theme option
        document.querySelectorAll('.theme-option').forEach(opt => {
            opt.classList.toggle('active', opt.dataset.theme === theme);
        });

        // Save to localStorage
        if (save) {
            localStorage.setItem('circuitAppTheme', theme);
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new CircuitApp();
});
