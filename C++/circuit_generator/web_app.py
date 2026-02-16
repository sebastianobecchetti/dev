#!/usr/bin/env python3
"""
Flask backend for Circuit Tutor Web App
Serves modern web interface and handles circuit generation
"""

from flask import Flask, render_template, jsonify, send_file
import subprocess
import json
import os

app = Flask(__name__)

GENERATOR_PATH = "./circuit_generator"
SVG_PATH = "circuit.svg"
PNG_PATH = "circuit.png"

@app.route('/')
def index():
    """Serve the main web interface"""
    return render_template('index.html')

@app.route('/api/generate/<exercise_type>')
def generate_circuit(exercise_type):
    """Generate a new circuit and return JSON with solution"""
    try:
        # Validate exercise type
        if exercise_type not in ['DC', 'AC']:
            return jsonify({'error': 'Invalid exercise type'}), 400
        
        # Run C++ generator
        result = subprocess.run(
            [GENERATOR_PATH, "headless", exercise_type],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode != 0:
            return jsonify({'error': f'Generator failed: {result.stderr}'}), 500
        
        # Parse JSON output
        output = result.stdout.strip()
        start = output.find("{")
        end = output.rfind("}") + 1
        
        if start == -1 or end == 0:
            return jsonify({'error': 'No JSON found in output'}), 500
        
        solution = json.loads(output[start:end])
        
        # Convert SVG to PNG
        subprocess.run(
            ["rsvg-convert", SVG_PATH, "-o", PNG_PATH],
            check=True,
            timeout=5
        )
        
        return jsonify({
            'success': True,
            'solution': solution,
            'svg_path': SVG_PATH,
            'png_path': PNG_PATH
        })
        
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Generation timeout'}), 500
    except json.JSONDecodeError as e:
        return jsonify({'error': f'Invalid JSON: {str(e)}'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/circuit.png')
def get_circuit_image():
    """Serve the circuit PNG image"""
    if os.path.exists(PNG_PATH):
        return send_file(PNG_PATH, mimetype='image/png')
    return jsonify({'error': 'Image not found'}), 404

@app.route('/api/check_answer', methods=['POST'])
def check_answer():
    """Check if user's answer is correct"""
    from flask import request
    
    data = request.get_json()
    user_value = data.get('value')
    correct_value = data.get('correct')
    
    try:
        user_val = float(user_value)
        correct_val = float(correct_value)
        
        diff = abs(user_val - correct_val)
        denom = abs(correct_val) if abs(correct_val) > 1e-9 else 1.0
        error = diff / denom if abs(correct_val) > 1e-9 else diff
        
        is_correct = error < 0.05
        
        return jsonify({
            'correct': is_correct,
            'error_percent': error * 100,
            'expected': correct_val
        })
        
    except (ValueError, TypeError) as e:
        return jsonify({'error': 'Invalid number'}), 400

if __name__ == '__main__':
    # Run on localhost:5000
    print("🚀 Starting Circuit Tutor Web Server...")
    print("📱 Open http://localhost:5000 in your browser")
    app.run(debug=True, host='0.0.0.0', port=5000)
