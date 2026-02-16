#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <set>
#include <fstream>
#include <cmath>
#include <map>

// Constants for Drawing
const int GRID_SIZE = 100; // Pixels between nodes
const int MARGIN = 50;

// Component types
enum ComponentType {
    WIRE, // Special type for connections
    RESISTOR,
    VOLTAGE_SOURCE,
    CURRENT_SOURCE,
    CAPACITOR,
    INDUCTOR,
    SWITCH
};

// Exercise types
enum ExerciseType {
    DC_TRANSIENT,
    AC_STEADY_STATE
};

// Complex number for phasor calculations
struct Complex {
    double real, imag;
    
    Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}
    
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }
    
    Complex operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag);
    }
    
    Complex operator*(const Complex& other) const {
        return Complex(real * other.real - imag * other.imag,
                      real * other.imag + imag * other.real);
    }
    
    Complex operator/(const Complex& other) const {
        double denom = other.real * other.real + other.imag * other.imag;
        if (std::abs(denom) < 1e-12) return Complex(0, 0);
        return Complex((real * other.real + imag * other.imag) / denom,
                      (imag * other.real - real * other.imag) / denom);
    }
    
    Complex operator*(double scalar) const {
        return Complex(real * scalar, imag * scalar);
    }
    
    Complex operator/(double scalar) const {
        if (std::abs(scalar) < 1e-12) return Complex(0, 0);
        return Complex(real / scalar, imag / scalar);
    }
    
    double magnitude() const {
        return std::sqrt(real * real + imag * imag);
    }
    
    double phase() const {
        return std::atan2(imag, real);
    }
    
    Complex conjugate() const {
        return Complex(real, -imag);
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3);
        if (std::abs(imag) < 1e-9) {
            ss << real;
        } else if (std::abs(real) < 1e-9) {
            ss << imag << "j";
        } else {
            ss << real << (imag >= 0 ? " + " : " - ") << std::abs(imag) << "j";
        }
        return ss.str();
    }
};

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator<(const Point& other) const { return x < other.x || (x == other.x && y < other.y); }
};

// Component structure
struct Component {
    std::string name;
    ComponentType type;
    int nodeA_idx;
    int nodeB_idx;
    Point pA; // Coordinate of Node A
    Point pB; // Coordinate of Node B
    double value;
    
    // Switch properties
    bool startsOpen = false; // If true: Open at t<0, Closes at t=0. If false: Closed at t<0, Opens at t=0.
    
    // AC exercise properties
    bool highlightRed = false; // If true, draw resistor in red for power calculation questions

    std::string getTypeString() const {
        switch (type) {
            case RESISTOR: return "R";
            case VOLTAGE_SOURCE: return "V";
            case CURRENT_SOURCE: return "I";
            case CAPACITOR: return "C";
            case INDUCTOR: return "L";
            case SWITCH: return "S";
            case WIRE: return "W";
            default: return "?";
        }
    }

    std::string getValueLabel() const {
        std::stringstream ss;
        if (type == RESISTOR) ss << value << " Ohm";
        else if (type == VOLTAGE_SOURCE) ss << value << " V";
        else if (type == CURRENT_SOURCE) ss << value << " A";
        else if (type == CAPACITOR) ss << value << " uF";
        else if (type == INDUCTOR) ss << value << " mH";
        else if (type == SWITCH) ss << (startsOpen ? "t=0 Close" : "t=0 Open");
        return ss.str();
    }

    std::string toString() const {
        std::stringstream ss;
        if (type == WIRE) return "Wire (" + std::to_string(nodeA_idx) + "-" + std::to_string(nodeB_idx) + ")";
        ss << name << " " << "N" << nodeA_idx << " " << "N" << nodeB_idx << " ";
        if (type == RESISTOR) ss << value << " Ohm";
        else if (type == VOLTAGE_SOURCE) ss << value << " V";
        else if (type == CURRENT_SOURCE) ss << value << " A";
        else if (type == CAPACITOR) ss << value << " uF";
        else if (type == INDUCTOR) ss << value << " mH";
        else if (type == SWITCH) ss << (startsOpen ? "Open->Close" : "Close->Open");
        return ss.str();
    }
};

// ... (Matrix class omitted here, included in file) ...
// ... (Circuit class omitted here) ...

// (Inside Circuit::drawComponent - we need to target the drawComponent function specifically)
// But wait, I can replacements in chunks. The user asked to modify the component struct and enum. 
// I will provide the FULL Component struct override above. 
// Now I also need to provide the drawing logic update.
// I will split this into two tool calls or target specific areas.
// The Component struct replacement above covers Lines 18-76.
// I'll stick to that for this tool call.


// --- Matrix Helper Class for Solver ---
class Matrix {
public:
    int rows, cols;
    std::vector<std::vector<double>> data;

    Matrix(int r, int c) : rows(r), cols(c) {
        data.resize(r, std::vector<double>(c, 0.0));
    }

    // Accessor
    double& at(int r, int c) { return data[r][c]; }
    const double& at(int r, int c) const { return data[r][c]; }

    // Gaussian Elimination to solve Ax = b
    // Returns x vector. Assumes A is square (N x N) and b is N x 1
    static std::vector<double> solve(Matrix A, std::vector<double> b) {
        int n = A.rows;
        // Augmented matrix [A | b]
        for (int i = 0; i < n; i++) {
            A.data[i].push_back(b[i]);
        }

        // Forward Elimination
        for (int i = 0; i < n; i++) {
            // Pivoting
            int pivot = i;
            for (int j = i + 1; j < n; j++) {
                if (std::abs(A.data[j][i]) > std::abs(A.data[pivot][i])) pivot = j;
            }
            std::swap(A.data[i], A.data[pivot]);

            if (std::abs(A.data[i][i]) < 1e-9) continue; // Singular or nearly singular

            for (int j = i + 1; j < n; j++) {
                double factor = A.data[j][i] / A.data[i][i];
                for (int k = i; k <= n; k++) {
                    A.data[j][k] -= factor * A.data[i][k];
                }
            }
        }

        // Back Substitution
        std::vector<double> x(n);
        for (int i = n - 1; i >= 0; i--) {
            double sum = 0;
            for (int j = i + 1; j < n; j++) {
                sum += A.data[i][j] * x[j];
            }
            if (std::abs(A.data[i][i]) > 1e-9)
                x[i] = (A.data[i][n] - sum) / A.data[i][i];
            else 
                x[i] = 0; // Or handle error
        }
        return x;
    }
};

class Circuit {
public:
    std::vector<Point> nodes;
    std::vector<Component> components;
    int width, height; // Grid dimensions
    
    // Exercise State
    ExerciseType exerciseType;
    double omega;              // Angular frequency (rad/s) for AC
    double frequency;          // Frequency (Hz) for AC
    std::string sourceWaveform; // "sin" or "cos" for AC exercises
    int sourceAmplitude;       // Amplitude in Amps (integer value like 1, 2, 5, 10)
    std::string questionText;
    
    std::vector<Component*> targetResistors; // Resistors for power calculation (AC)
    Component* targetComp;     // Target component for transient analysis (DC)
    int questionType = -1; // 0=V, 1=I, 2=P, 3=E(Energy), 4=Q(Charge)

    Circuit(int w, int h) : width(w), height(h), exerciseType(DC_TRANSIENT), 
                            omega(0), frequency(0), sourceWaveform("sin"), sourceAmplitude(1), targetComp(nullptr) {
        // Initialize grid nodes
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                nodes.push_back({x, y});
            }
        }
    }

    void addComponent(Component c) {
        components.push_back(c);
    }
    
    void generateQuestion() {
        // Find suitable components for questions (R, L, C)
        std::vector<Component*> targets;
        for (auto& c : components) {
            if (c.type == RESISTOR || c.type == CAPACITOR || c.type == INDUCTOR) {
                targets.push_back(&c);
            }
        }
        
        if (targets.empty()) {
            questionText = "Calculate the total power supplied by the sources.";
            return;
        }
        
        targetComp = targets[rand() % targets.size()]; // Point to element in vector (WARNING: reallocation invalidates ptr)
        // Better store index, but vector resizing shouldn't happen after generation.
        // Actually, let's fix the pointer issue by using index or just ensuring no push_backs happen after.
        // Let's store pointer, but be careful.
        
        // Pick type
        int type = targetComp->type;
        std::stringstream ss;
        
        if (type == RESISTOR) {
            questionType = rand() % 3; // 0=V, 1=I, 2=P
            if (questionType == 0) ss << "Calculate the Voltage (V) across " << targetComp->name << ".";
            else if (questionType == 1) ss << "Calculate the Current (I) flowing through " << targetComp->name << ".";
            else ss << "Calculate the Power (P) dissipated by " << targetComp->name << ".";
        } else if (type == CAPACITOR) {
            questionType = rand() % 3; // 0=V, 1=Q, 2=E
            if (questionType == 0) ss << "Calculate the DC Steady State Voltage across " << targetComp->name << ".";
            else if (questionType == 1) { ss << "Calculate the Charge (Q) stored in " << targetComp->name << " at DC Steady State."; questionType = 4; } // Map to 4 for Q
            else { ss << "Calculate the Energy (E) stored in " << targetComp->name << " at DC Steady State."; questionType = 3; } // Map to 3 for E
        } else if (type == INDUCTOR) {
            questionType = rand() % 3; // 0=I, 1=V(0), 2=E
            if (questionType == 0) { ss << "Calculate the DC Steady State Current through " << targetComp->name << "."; questionType = 1; } // I
            else if (questionType == 1) { ss << "Calculate the Voltage across " << targetComp->name << " at DC Steady State."; questionType = 0; } // V (should be 0)
            else { ss << "Calculate the Energy (E) stored in " << targetComp->name << " at DC Steady State."; questionType = 3; } // E
        }
        
        questionText = ss.str();
    }
    
    // MNA Solver for DC Steady State
    // switchState: true = initial state (t<0), false = final state (t>0)
    // killSources: if true, turn off all V/I sources (for Thevenin)
    // injectCurrent: if > 0, injects this current into targetComp nodes (for Thevenin)
    std::vector<double> solveMNA(bool switchStateInitial, bool killSources, double injectCurrent = 0.0) {
        int numNodes = nodes.size();
        std::vector<int> voltSourceIndices;
        
        // Identify Voltage Sources (Component Indices)
        for(size_t i=0; i<components.size(); ++i) {
            // If killing sources, treating V source as Short (0V) -> but MNA handles 0V source fine.
            // Actually, if killing sources, V source becomes Wire. 
            // Better to keep them as "Voltage Source with 0V".
            if(components[i].type == VOLTAGE_SOURCE) voltSourceIndices.push_back(i);
        }
        int numV = voltSourceIndices.size();
        
        int mSize = numNodes - 1 + numV; 
        Matrix A(mSize, mSize);
        std::vector<double> z(mSize, 0.0);

        auto getIdx = [](int n) { return n - 1; };

        // Fill Conductances
        for(const auto& c : components) {
            double g = 0.0;
            
            if (c.type == RESISTOR) {
                g = 1.0 / c.value;
            } else if (c.type == INDUCTOR) {
                // DC Short
                g = 1.0 / 1e-6; 
            } else if (c.type == CAPACITOR) {
                // DC Open
                g = 0.0; 
            } else if (c.type == SWITCH) {
                // Check state
                bool isOpen = c.startsOpen; 
                if (!switchStateInitial) isOpen = !isOpen; // Flip state for t>0

                if (isOpen) g = 0.0;
                else g = 1.0 / 1e-6; // Closed = Short
            } else if (c.type == CURRENT_SOURCE) {
                if (killSources) continue; // It's 0A -> Open circuit -> Ignore
                
                int nA = c.nodeA_idx;
                int nB = c.nodeB_idx;
                if (nA > 0) z[getIdx(nA)] -= c.value;
                if (nB > 0) z[getIdx(nB)] += c.value;
                continue; 
            } else {
                continue; // V sources handled below
            }

            if (g > 0.0) {
                int nA = c.nodeA_idx;
                int nB = c.nodeB_idx;
                if (nA > 0) A.at(getIdx(nA), getIdx(nA)) += g;
                if (nB > 0) A.at(getIdx(nB), getIdx(nB)) += g;
                if (nA > 0 && nB > 0) {
                    A.at(getIdx(nA), getIdx(nB)) -= g;
                    A.at(getIdx(nB), getIdx(nA)) -= g;
                }
            }
        }
        
        // Test Current Injection (Thevenin)
        if (injectCurrent != 0.0 && targetComp) {
            // Inject 1A from B to A (Active sign convention measures V_A - V_B)
            // Or typically we inject INTO A and OUT of B to measure R = (Va - Vb)/I.
            int nA = targetComp->nodeA_idx;
            int nB = targetComp->nodeB_idx;
            // Inject INTO nA (+I), OUT of nB (-I)
            if (nA > 0) z[getIdx(nA)] += injectCurrent;
            if (nB > 0) z[getIdx(nB)] -= injectCurrent; // Wait, MNA RHS is sum of currents entering.
            // If Source is external injecting INTO node A, then it adds to RHS. Correct.
        }

        // Voltage Sources
        for (int i = 0; i < numV; ++i) {
            int compIdx = voltSourceIndices[i];
            const auto& c = components[compIdx];
            int row = numNodes - 1 + i;
            
            int nA = c.nodeA_idx;
            int nB = c.nodeB_idx;
            
            if (nA > 0) {
                A.at(row, getIdx(nA)) = 1;
                A.at(getIdx(nA), row) = 1;
            }
            if (nB > 0) {
                A.at(row, getIdx(nB)) = -1;
                A.at(getIdx(nB), row) = -1;
            }
            
            // If killSources is true, all V sources become 0V wires
            z[row] = killSources ? 0.0 : c.value;
        }

        std::vector<double> x = Matrix::solve(A, z);
        
        // Extract Node Voltages
        std::vector<double> V_nodes(numNodes, 0.0);
        for(int i=1; i<numNodes; ++i) V_nodes[i] = x[getIdx(i)];
        
        return V_nodes;
    }

    // Helper to get voltage/current of target
    double getComponentValue(const std::vector<double>& V_nodes, Component* target, bool isInductor) {
         double v = V_nodes[target->nodeA_idx] - V_nodes[target->nodeB_idx];
         if (isInductor) {
             // For inductor in DC, I = V/R_small? Or we need the branch current?
             // MNA variables for voltage sources give currents directly.
             // But L is modeled as discrete resistor 1e-6.
             // So I = V / 1e-6.
             return v / 1e-6; // Approx I
         } else {
             // Capacitor -> V
             return v;
         }
    }

    struct TransientResult {
        double initialVal;
        double finalVal;
        double tau;
    };

    TransientResult solveTransient() {
        if (!targetComp) return {0,0,0};
        
        bool isInductor = (targetComp->type == INDUCTOR);

        // 1. Initial State (t < 0)
        // Switch is in 'startsOpen' state.
        std::vector<double> vNodesInit = solveMNA(true, false);
        double valInit = getComponentValue(vNodesInit, targetComp, isInductor);
        
        // 2. Final State (t = inf)
        // Switch is in '!startsOpen' state.
        std::vector<double> vNodesFinal = solveMNA(false, false);
        double valFinal = getComponentValue(vNodesFinal, targetComp, isInductor);
        
        // 3. Time Constant (Tau)
        // State: t > 0 (Final State switch)
        // Sources: Killed.
        // Component: Removed (Open).
        // Injection: 1A at component terminals.
        // Req = V_terminals / 1A.
        
        // Important: When calculating Req seen by L/C, the L/C itself must be removed from the circuit.
        // My solveMNA counts L/C as Short/Open based on DC rules. 
        // For Req Calculation, we want L/C to be "Open" (removed) so we can measure resistance across valid terminals.
        // My MNA Loop:
        // if c.type == CAPACITOR -> g=0 (already open).
        // if c.type == INDUCTOR -> g=large (Short).
        // I need to force the targetComp to be OPEN during Req calculation even if it is an Inductor.
        // I'll assume targetComp 'g' contribution is 0 manually or handle it.
        // Wait, MNA loop iterates all components.
        // I should have a flag 'excludeTarget' in solveMNA? Or just temporarily set its type/value.
        // Let's modify solveMNA to ignore targetComp.
        
        // Actually, let's keep it simple:
        // Temporarily change targetComp type to CAPACITOR (Open) if it's INDUCTOR, 
        // calculate, then swap back.
        ComponentType oldType = targetComp->type;
        targetComp->type = CAPACITOR; // Force Open
        
        std::vector<double> vNodesReq = solveMNA(false, true, 1.0); // t>0 switch, kill sources, inject 1A
        
        targetComp->type = oldType; // Restore
        
        double vTh = vNodesReq[targetComp->nodeA_idx] - vNodesReq[targetComp->nodeB_idx];
        double Req = std::abs(vTh / 1.0);
        
        double tau = 0.0;
        if (isInductor) {
            // L / R
            // targetComp->value is mH -> 1e-3 H
            tau = (targetComp->value * 1e-3) / Req;
        } else {
            // R * C
            // targetComp->value is uF -> 1e-6 F
            tau = Req * (targetComp->value * 1e-6);
        }
        
        return {valInit, valFinal, tau};
    }

    // ============ AC Analysis Methods ============
    
    struct ACResult {
        std::map<std::string, double> avgPower;  // resistor name -> power (W)
        double powerFactor;
        bool hasPowerFactor;  // true if single source exists
    };
    
    // Calculate impedance in phasor domain
    Complex calculateImpedance(const Component& c, double omega_val) {
        if (c.type == RESISTOR) {
            return Complex(c.value, 0);  // Z = R
        } else if (c.type == INDUCTOR) {
            // Z = jωL, L in mH -> H
            double L_henry = c.value * 1e-3;
            return Complex(0, omega_val * L_henry);  // jωL
        } else if (c.type == CAPACITOR) {
            // Z = 1/(jωC) = -j/(ωC), C in µF -> F
            double C_farad = c.value * 1e-6;
            return Complex(0, -1.0 / (omega_val * C_farad));  // -j/(ωC)
        }
        return Complex(0, 0);
    }
    
    // Simple complex linear system solver
    std::vector<Complex> solveComplexLinearSystem(std::vector<std::vector<Complex>> A, std::vector<Complex> b) {
        int n = A.size();
        
        // Augment matrix
        for (int i = 0; i < n; i++) {
            A[i].push_back(b[i]);
        }
        
        // Forward elimination
        for (int i = 0; i < n; i++) {
            // Pivoting
            int pivot = i;
            for (int j = i + 1; j < n; j++) {
                if (A[j][i].magnitude() > A[pivot][i].magnitude()) pivot = j;
            }
            std::swap(A[i], A[pivot]);
            
            if (A[i][i].magnitude() < 1e-12) continue;
            
            for (int j = i + 1; j < n; j++) {
                Complex factor = A[j][i] / A[i][i];
                for (int k = i; k <= n; k++) {
                    A[j][k] = A[j][k] - factor * A[i][k];
                }
            }
        }
        
        // Back substitution
        std::vector<Complex> x(n);
        for (int i = n - 1; i >= 0; i--) {
            Complex sum(0, 0);
            for (int j = i + 1; j < n; j++) {
                sum = sum + A[i][j] * x[j];
            }
            if (A[i][i].magnitude() > 1e-12) {
                x[i] = (A[i][n] - sum) / A[i][i];
            } else {
                x[i] = Complex(0, 0);
            }
        }
        return x;
    }
    
    // Solve AC circuit using complex MNA
    ACResult solveAC() {
        ACResult result;
        result.hasPowerFactor = false;
        result.powerFactor = 0.0;
        
        int numNodes = nodes.size();
        
        // Count voltage sources for MNA size
        std::vector<int> voltSourceIndices;
        int numSources = 0;
        for(size_t i=0; i<components.size(); ++i) {
            if(components[i].type == VOLTAGE_SOURCE) {
                voltSourceIndices.push_back(i);
                numSources++;
            } else if(components[i].type == CURRENT_SOURCE) {
                numSources++;
            }
        }
        
        // Check if we have exactly one source for power factor
        result.hasPowerFactor = (numSources == 1);
        
        int numV = voltSourceIndices.size();
        int mSize = numNodes - 1 + numV;
        
        // Complex MNA matrices
        std::vector<std::vector<Complex>> A(mSize, std::vector<Complex>(mSize, Complex(0, 0)));
        std::vector<Complex> z(mSize, Complex(0, 0));
        
        auto getIdx = [](int n) { return n - 1; };
        
        // Fill complex admittances
        for(const auto& c : components) {
            Complex Y(0, 0);  // Admittance
            
            if (c.type == RESISTOR || c.type == INDUCTOR || c.type == CAPACITOR) {
                Complex Z = calculateImpedance(c, omega);
                if (Z.magnitude() > 1e-12) {
                    Y = Complex(1, 0) / Z;  // Y = 1/Z
                }
            } else if (c.type == CURRENT_SOURCE) {
                // Current sources contribute to RHS
                int nA = c.nodeA_idx;
                int nB = c.nodeB_idx;
                // Assuming AC source with amplitude c.value and phase 0
                Complex I_source(c.value, 0);
                if (nA > 0) z[getIdx(nA)] = z[getIdx(nA)] - I_source;
                if (nB > 0) z[getIdx(nB)] = z[getIdx(nB)] + I_source;
                continue;
            } else {
                continue; // V sources handled below
            }
            
            // Stamp admittance
            if (Y.magnitude() > 1e-12) {
                int nA = c.nodeA_idx;
                int nB = c.nodeB_idx;
                if (nA > 0) A[getIdx(nA)][getIdx(nA)] = A[getIdx(nA)][getIdx(nA)] + Y;
                if (nB > 0) A[getIdx(nB)][getIdx(nB)] = A[getIdx(nB)][getIdx(nB)] + Y;
                if (nA > 0 && nB > 0) {
                    A[getIdx(nA)][getIdx(nB)] = A[getIdx(nA)][getIdx(nB)] - Y;
                    A[getIdx(nB)][getIdx(nA)] = A[getIdx(nB)][getIdx(nA)] - Y;
                }
            }
        }
        
        // Voltage sources (same MNA technique but with complex values)
        for (int i = 0; i < numV; ++i) {
            int compIdx = voltSourceIndices[i];
            const auto& c = components[compIdx];
            int row = numNodes - 1 + i;
            
            int nA = c.nodeA_idx;
            int nB = c.nodeB_idx;
            
            if (nA > 0) {
                A[row][getIdx(nA)] = Complex(1, 0);
                A[getIdx(nA)][row] = Complex(1, 0);
            }
            if (nB > 0) {
                A[row][getIdx(nB)] = Complex(-1, 0);
                A[getIdx(nB)][row] = Complex(-1, 0);
            }
            
            // Voltage source phasor (amplitude with phase 0)
            z[row] = Complex(c.value, 0);
        }
        
        // Solve complex system (Gaussian elimination)
        std::vector<Complex> x = solveComplexLinearSystem(A, z);
        
        // Extract node voltages
        std::vector<Complex> V_nodes(numNodes, Complex(0, 0));
        for(int i=1; i<numNodes; ++i) {
            if (getIdx(i) < (int)x.size()) {
                V_nodes[i] = x[getIdx(i)];
            }
        }
        
        // Calculate powers for target resistors
        for (Component* targetR : targetResistors) {
            if (targetR && targetR->type == RESISTOR) {
                Complex V_comp = V_nodes[targetR->nodeA_idx] - V_nodes[targetR->nodeB_idx];
                double V_mag = V_comp.magnitude();
                // P_avg = |V|^2 / (2*R)
                double P_avg = (V_mag * V_mag) / (2.0 * targetR->value);
                result.avgPower[targetR->name] = P_avg;
            }
        }
        
        // Calculate power factor if single source
        if (result.hasPowerFactor) {
            // Find the source
            Component* source = nullptr;
            int sourceType = -1; // 0=V, 1=I
            for (auto& c : components) {
                if (c.type == VOLTAGE_SOURCE) {
                    source = &c;
                    sourceType = 0;
                    break;
                } else if (c.type == CURRENT_SOURCE) {
                    source = &c;
                    sourceType = 1;
                    break;
                }
            }
            
            if (source) {
                Complex V_source, I_source;
                
                if (sourceType == 0) {
                    // Voltage source: V is known, I is in MNA solution
                    V_source = Complex(source->value, 0);
                    // Current through V source is the corresponding MNA variable
                    // It's at position numNodes-1 (first voltage source)
                    if ((int)x.size() > numNodes - 1) {
                        I_source = x[numNodes - 1];
                    }
                } else {
                    // Current source: I is known, V across it is node difference
                    I_source = Complex(source->value, 0);
                    V_source = V_nodes[source->nodeA_idx] - V_nodes[source->nodeB_idx];
                }
                
                // Power factor = cos(angle(V) - angle(I))
                double phi = V_source.phase() - I_source.phase();
                result.powerFactor = std::cos(phi);
            }
        }
        
        return result;
    }

    void exportSVG(const std::string& filename) {
        std::ofstream svg(filename);
        int gridWidth = (width - 1) * GRID_SIZE + 2 * MARGIN;
        int gridHeight = (height - 1) * GRID_SIZE + 2 * MARGIN;
        
        // Ensure minimum width for text
        int imgWidth = std::max(gridWidth, 500);
        int imgHeight = gridHeight + 40; // Extra space for text

        svg << "<svg width=\"" << imgWidth << "\" height=\"" << imgHeight << "\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
        
        // Definitions
        svg << "<defs>" << std::endl;
        svg << "<marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"10\" refX=\"9\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">" << std::endl;
        svg << "<path d=\"M0,0 L0,6 L9,3 z\" fill=\"black\" />" << std::endl;
        svg << "</marker>" << std::endl;
        svg << "</defs>" << std::endl;
        
        svg << "<style>" << std::endl;
        svg << ".wire { stroke: black; stroke-width: 2; fill: none; }" << std::endl;
        svg << ".node { fill: black; }" << std::endl;
        svg << ".text { font-family: sans-serif; font-size: 12px; }" << std::endl;
        svg << ".qtext { font-family: sans-serif; font-size: 14px; font-weight: bold; fill: #0000AA; }" << std::endl;
        svg << "</style>" << std::endl;
        
        // Background
        svg << "<rect width=\"100%\" height=\"100%\" fill=\"white\" />" << std::endl;

        // Draw Components
        for (const auto& c : components) {
            drawComponent(svg, c);
        }

        // Draw Nodes (Dots)
        for (const auto& p : nodes) {
            bool used = false;
            for(const auto& c : components) {
                if(c.pA == p || c.pB == p) { used = true; break; }
            }
            if (used) {
                int px = MARGIN + p.x * GRID_SIZE;
                int py = MARGIN + p.y * GRID_SIZE;
                svg << "<circle cx=\"" << px << "\" cy=\"" << py << "\" r=\"4\" class=\"node\" />" << std::endl;
            }
        }
        
        // Question text removed - shown in GUI instead
        
        svg << "</svg>" << std::endl;
        svg.close();
        std::cout << "Exported circuit to " << filename << std::endl;
    }

private:
    void drawComponent(std::ofstream& svg, const Component& c) {
        int x1 = MARGIN + c.pA.x * GRID_SIZE;
        int y1 = MARGIN + c.pA.y * GRID_SIZE;
        int x2 = MARGIN + c.pB.x * GRID_SIZE;
        int y2 = MARGIN + c.pB.y * GRID_SIZE;

        int mx = (x1 + x2) / 2;
        int my = (y1 + y2) / 2;

        // Check orientation
        bool isHorizontal = (y1 == y2);
        
        if (c.type == WIRE) {
            svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\" class=\"wire\" />" << std::endl;
            return;
        }

        // Clip the line in the middle specifically for symbol
        // We want to draw a line from p1 to start of symbol, and end of symbol to p2
        // Symbol size approx 30px
        int gap = 15;
        
        int sx1, sy1, sx2, sy2; // Symbol Drawing Coordinates
        if (isHorizontal) {
            int dx = (x2 > x1) ? 1 : -1;
            sx1 = mx - gap * dx; sy1 = my;
            sx2 = mx + gap * dx; sy2 = my;
            // Wires to symbol
            svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << sx1 << "\" y2=\"" << sy1 << "\" class=\"wire\" />" << std::endl;
            svg << "<line x1=\"" << sx2 << "\" y1=\"" << sy2 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\" class=\"wire\" />" << std::endl;
        } else {
            int dy = (y2 > y1) ? 1 : -1;
            sx1 = mx; sy1 = my - gap * dy;
            sx2 = mx; sy2 = my + gap * dy;
            // Wires to symbol
            svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << sx1 << "\" y2=\"" << sy1 << "\" class=\"wire\" />" << std::endl;
            svg << "<line x1=\"" << sx2 << "\" y1=\"" << sy2 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\" class=\"wire\" />" << std::endl;
        }

        // Draw Symbol
        if (c.type == RESISTOR) {
            // Choose color based on highlightRed flag
            std::string color = c.highlightRed ? "red" : "black";
            
            // Zigzag
            svg << "<path d=\"";
            if (isHorizontal) {
                 // 6 segments
                 double step = (double)(sx2 - sx1) / 6.0;
                 svg << "M " << sx1 << " " << sy1 << " ";
                 for(int i=1; i<=6; i++) {
                     double tx = sx1 + i * step;
                     double ty = sy1 + ((i%2==0) ? -5 : 5) * ((i==6) ? 0 : 1); // Zigzag offset
                     if (i==6) ty = sy2; // Snap to end
                     svg << "L " << tx << " " << ty << " ";
                 }
            } else {
                 double step = (double)(sy2 - sy1) / 6.0;
                 svg << "M " << sx1 << " " << sy1 << " ";
                 for(int i=1; i<=6; i++) {
                     double ty = sy1 + i * step;
                     double tx = sx1 + ((i%2==0) ? -5 : 5) * ((i==6) ? 0 : 1);
                     if (i==6) tx = sx2;
                     svg << "L " << tx << " " << ty << " ";
                 }
            }
            svg << "\" stroke=\"" << color << "\" stroke-width=\"2\" fill=\"none\" />" << std::endl;
            
            // Text Label
            svg << "<text x=\"" << (mx + (isHorizontal ? 0 : 10)) << "\" y=\"" << (my + (isHorizontal ? -10 : 0)) << "\" class=\"text\">" << c.getValueLabel() << "</text>" << std::endl;

        } else if (c.type == VOLTAGE_SOURCE) {
            // Circle
            svg << "<circle cx=\"" << mx << "\" cy=\"" << my << "\" r=\"" << gap << "\" class=\"wire\" />" << std::endl;
            // Name label - use Value
             svg << "<text x=\"" << (mx + (isHorizontal ? 0 : 15)) << "\" y=\"" << (my + (isHorizontal ? -15 : 0)) << "\" class=\"text\">" << c.getValueLabel() << "</text>" << std::endl;
             
             // + and - signs inside the circle, oriented towards nodes
             if (isHorizontal) {
                 int dir = (x1 < x2) ? -1 : 1;
                 svg << "<text x=\"" << mx + (dir * 7) - 3 << "\" y=\"" << my + 4 << "\" font-size=\"12\" font-weight=\"bold\">+</text>";
                 svg << "<text x=\"" << mx - (dir * 7) - 3 << "\" y=\"" << my + 4 << "\" font-size=\"12\" font-weight=\"bold\">-</text>";
             } else {
                 int dir = (y1 < y2) ? -1 : 1;
                 svg << "<text x=\"" << mx - 3 << "\" y=\"" << my + (dir * 7) + 3 << "\" font-size=\"12\" font-weight=\"bold\">+</text>";
                 svg << "<text x=\"" << mx - 3 << "\" y=\"" << my - (dir * 7) + 3 << "\" font-size=\"12\" font-weight=\"bold\">-</text>";
             }

        } else if (c.type == CURRENT_SOURCE) {
            svg << "<circle cx=\"" << mx << "\" cy=\"" << my << "\" r=\"" << gap << "\" class=\"wire\" />" << std::endl;
             svg << "<text x=\"" << (mx + (isHorizontal ? 0 : 15)) << "\" y=\"" << (my + (isHorizontal ? -15 : 0)) << "\" class=\"text\">" << c.getValueLabel() << "</text>" << std::endl;
            // Arrow (simple line)
             if (isHorizontal) {
                  svg << "<line x1=\"" << mx - 8 << "\" y1=\"" << my << "\" x2=\"" << mx + 8 << "\" y2=\"" << my << "\" class=\"wire\" />";
                  svg << "<path d=\"M " << mx+4 << " " << my-4 << " L " << mx+8 << " " << my << " L " << mx+4 << " " << my+4 << "\" class=\"wire\" />";
             } else {
                  svg << "<line x1=\"" << mx << "\" y1=\"" << my - 8 << "\" x2=\"" << mx << "\" y2=\"" << my + 8 << "\" class=\"wire\" />";
                  svg << "<path d=\"M " << mx-4 << " " << my+4 << " L " << mx << " " << my+8 << " L " << mx+4 << " " << my+4 << "\" class=\"wire\" />";
             }
        } else if (c.type == CAPACITOR) {
            // Two parallel plates
            // Gap is 15px. We need two lines separated by maybe 6px
            int plateSep = 4;
            int plateLen = 20;

            if (isHorizontal) {
                 // Lines at mx +/- plateSep
                 // Vertical plates
                 svg << "<line x1=\"" << mx - plateSep << "\" y1=\"" << my - plateLen/2 << "\" x2=\"" << mx - plateSep << "\" y2=\"" << my + plateLen/2 << "\" class=\"wire\" />";
                 svg << "<line x1=\"" << mx + plateSep << "\" y1=\"" << my - plateLen/2 << "\" x2=\"" << mx + plateSep << "\" y2=\"" << my + plateLen/2 << "\" class=\"wire\" />";
                 // Connecting wires
                 svg << "<line x1=\"" << sx1 << "\" y1=\"" << sy1 << "\" x2=\"" << mx - plateSep << "\" y2=\"" << sy1 << "\" class=\"wire\" />";
                 svg << "<line x1=\"" << sx2 << "\" y1=\"" << sy2 << "\" x2=\"" << mx + plateSep << "\" y2=\"" << sy2 << "\" class=\"wire\" />";
            } else {
                 // Lines at my +/- plateSep
                 // Horizontal plates
                 svg << "<line x1=\"" << mx - plateLen/2 << "\" y1=\"" << my - plateSep << "\" x2=\"" << mx + plateLen/2 << "\" y2=\"" << my - plateSep << "\" class=\"wire\" />";
                 svg << "<line x1=\"" << mx - plateLen/2 << "\" y1=\"" << my + plateSep << "\" x2=\"" << mx + plateLen/2 << "\" y2=\"" << my + plateSep << "\" class=\"wire\" />";
                 // Connecting wires
                 svg << "<line x1=\"" << sx1 << "\" y1=\"" << sy1 << "\" x2=\"" << sx1 << "\" y2=\"" << my - plateSep << "\" class=\"wire\" />";
                 svg << "<line x1=\"" << sx2 << "\" y1=\"" << sy2 << "\" x2=\"" << sx2 << "\" y2=\"" << my + plateSep << "\" class=\"wire\" />";
            }
             svg << "<text x=\"" << (mx + (isHorizontal ? 0 : 15)) << "\" y=\"" << (my + (isHorizontal ? -15 : 0)) << "\" class=\"text\">" << c.getValueLabel() << "</text>" << std::endl;

        } else if (c.type == INDUCTOR) {
            // Coils (Bumps)
            int coilRadius = 5;
            int numCoils = 4;
            // Total length needed is roughly numCoils * 2 * radius
            // We fit it between sx1 and sx2 (which is 2*gap = 30px)
            // 4 coils * 10px = 40px might be too big. Let's do 3 coils aka 30px.

            svg << "<path d=\"M " << sx1 << " " << sy1 << " ";
            
            if (isHorizontal) {
                // Determine step direction
                int dx = (sx2 > sx1) ? 1 : -1;
                // Draw arcs
                double step = (double)(sx2 - sx1) / 3.0; // 3 coils
                for(int i=0; i<3; i++) {
                    double currX = sx1 + i * step;
                    double nextX = sx1 + (i + 1) * step;
                    // Cubic Bezier: control points up
                    // q control end matches quadratic
                    // svg arc: A rx ry rot large-arc-flag sweep-flag x y
                    // Let's use simple Q for bump
                    // Control point in middle-up
                    double cx = (currX + nextX) / 2.0;
                    double cy = sy1 - 10; // Bump up
                    svg << "Q " << cx << " " << cy << " " << nextX << " " << sy1 << " ";
                }
            } else {
                int dy = (sy2 > sy1) ? 1 : -1;
                 double step = (double)(sy2 - sy1) / 3.0;
                 for(int i=0; i<3; i++) {
                    double currY = sy1 + i * step;
                    double nextY = sy1 + (i + 1) * step;
                    double cy = (currY + nextY) / 2.0;
                    double cx = sx1 + 10; // Bump right
                    svg << "Q " << cx << " " << cy << " " << sx1 << " " << nextY << " ";
                 }
            }
            svg << "\" class=\"wire\" />" << std::endl;
             svg << "<text x=\"" << (mx + (isHorizontal ? 0 : 15)) << "\" y=\"" << (my + (isHorizontal ? -15 : 0)) << "\" class=\"text\">" << c.getValueLabel() << "</text>" << std::endl;
        
        } else if (c.type == SWITCH) {
            // Refined Switch Drawing
            // Gap is between sx1 and sx2.
            
            // 1. Terminals (Dots)
            svg << "<circle cx=\"" << sx1 << "\" cy=\"" << sy1 << "\" r=\"2.5\" class=\"node\" />" << std::endl;
            svg << "<circle cx=\"" << sx2 << "\" cy=\"" << sy2 << "\" r=\"2.5\" class=\"node\" />" << std::endl;
            
            // 2. Lever Geometry
            double leverLen = sqrt(pow(sx2-sx1,2) + pow(sy2-sy1,2)); // Distance
            // Base at sx1.
            
            double baseAngle = atan2(sy2-sy1, sx2-sx1); // Angle of the wire line
            
            double openAngleOffset = -0.6; // ~35 degrees counter-clock-wise
            
            double tipX, tipY;
            
            if (c.startsOpen) {
                // Draw Open Lever
                tipX = sx1 + leverLen * cos(baseAngle + openAngleOffset);
                tipY = sy1 + leverLen * sin(baseAngle + openAngleOffset);
                
                svg << "<line x1=\"" << sx1 << "\" y1=\"" << sy1 << "\" x2=\"" << tipX << "\" y2=\"" << tipY << "\" style=\"stroke:black; stroke-width:2;\" />" << std::endl;

            } else {
                // Draw Closed Lever
                svg << "<line x1=\"" << sx1 << "\" y1=\"" << sy1 << "\" x2=\"" << sx2 << "\" y2=\"" << sy2 << "\" style=\"stroke:black; stroke-width:2;\" />" << std::endl;
            }
            
            // Label "t=0 Opening/Closing"
            std::string label = c.startsOpen ? "t=0 Closing" : "t=0 Opening";
            
            // Position it above/below mid point
            double mx_lab = (sx1 + sx2)/2;
            double my_lab = (sy1 + sy2)/2;
            
            // Shift based on orientation/state to avoid overlap
            if (isHorizontal) {
                my_lab -= 20; 
            } else {
                mx_lab += 25; 
            }
            
            svg << "<text x=\"" << mx_lab << "\" y=\"" << my_lab << "\" class=\"text\" font-size=\"11\" text-anchor=\"middle\">" << label << "</text>" << std::endl;
        }
    }
};

// --- Generation Logic ---

Circuit generateGridCircuit() {
    // 2x2 or 3x2 grid
    int w = 3;
    int h = 3; // 3x3 grid = 9 nodes
    Circuit circuit(w, h);

    // Track edges to avoid duplicates
    std::vector<std::pair<int, int>> edges;
    std::set<std::pair<int, int>> existingEdges; // Stores indices

    // 1. Spanning Tree on Grid
    // Nodes are indexed 0..w*h-1
    int numNodes = w * h;
    std::vector<int> visited = {0};
    std::vector<int> unvisited;
    for(int i=1; i<numNodes; ++i) unvisited.push_back(i);

    auto addEdge = [&](int u, int v) {
        if (u > v) std::swap(u, v);
        if (existingEdges.find({u, v}) == existingEdges.end()) {
            existingEdges.insert({u, v});
            edges.push_back({u, v});
        }
    };

    // Helper to get neighbors on grid
    auto getNeighbors = [&](int u) {
        std::vector<int> n;
        int x = u % w;
        int y = u / w;
        if (x > 0) n.push_back(u - 1);
        if (x < w - 1) n.push_back(u + 1);
        if (y > 0) n.push_back(u - w);
        if (y < h - 1) n.push_back(u + w);
        return n;
    };

    // Randomized Prim's/DFS style growth
    while (!unvisited.empty()) {
        std::vector<std::pair<int, int>> candidates;
        for (int u : visited) {
            for (int v : getNeighbors(u)) {
                bool isUnvisited = false;
                for(int uv : unvisited) if(uv == v) isUnvisited = true;
                if (isUnvisited) {
                    candidates.push_back({u, v});
                }
            }
        }
        if (candidates.empty()) break;
        int idx = rand() % candidates.size();
        std::pair<int, int> edge = candidates[idx];
        addEdge(edge.first, edge.second);
        visited.push_back(edge.second);
        for(size_t i=0; i<unvisited.size(); ++i) {
            if (unvisited[i] == edge.second) {
                unvisited.erase(unvisited.begin() + i);
                break;
            }
        }
    }

    // 2. Add extra edges (Cycle forming)
    int cyclesToAdd = 4;
    for (int i=0; i<cyclesToAdd * 10; ++i) {
        if (cyclesToAdd <= 0) break;
        int u = rand() % numNodes;
        auto neighbors = getNeighbors(u);
        int v = neighbors[rand() % neighbors.size()];
        int u_s = std::min(u, v);
        int v_s = std::max(u, v);
        if (existingEdges.find({u_s, v_s}) == existingEdges.end()) {
            addEdge(u_s, v_s);
            cyclesToAdd--;
        }
    }

    // 3. Assign Components
    int rCount = 0, vCount = 0, iCount = 0, cCount = 0, lCount = 0;
    bool hasSource = false;
    bool hasSwitched = false;
    bool hasDynamic = false; // Has L or C
    
    // Decide if this is RL or RC circuit
    bool isRC = (rand() % 2 == 0);

    // To make it look "smart", try to put voltage sources on the perimeter
    auto isPerimeter = [&](int n) {
        int x = n % w;
        int y = n / w;
        return x==0 || x==w-1 || y==0 || y==h-1;
    };

    // Shuffle edges to randomize assignment order
    std::random_shuffle(edges.begin(), edges.end());

    for (const auto& edge : edges) {
        Component c;
        c.nodeA_idx = edge.first;
        c.nodeB_idx = edge.second;
        c.pA = circuit.nodes[edge.first];
        c.pB = circuit.nodes[edge.second];

        bool perimeterEdge = isPerimeter(edge.first) && isPerimeter(edge.second);
        
        // Priority 1: Dynamic Element (L or C) - Place exactly one
        if (!hasDynamic && (rand() % 10 < 3)) { // 30% chance to place if not yet placed
             if (isRC) {
                 c.type = CAPACITOR;
                 c.name = "C1";
                 c.value = 10.0 * (rand()%10 + 1); // uF
             } else {
                 c.type = INDUCTOR;
                 c.name = "L1";
                 c.value = 1.0 * (rand()%10 + 1); // mH
             }
             hasDynamic = true;
             circuit.addComponent(c);
             continue;
        }
        
        // Priority 2: Switch - Place exactly one
        if (!hasSwitched && (rand() % 10 < 3)) {
            c.type = SWITCH;
            c.name = "Sw1";
            c.value = 0; // No value
            c.startsOpen = (rand() % 2 == 0);
            hasSwitched = true;
             circuit.addComponent(c);
             continue;
        }

        int typeRoll = rand() % 100;
        
        if (!hasSource && perimeterEdge && (rand()%2==0)) {
             // Check total source count (vCount + iCount)
             if (vCount + iCount < 2) {
                 c.type = VOLTAGE_SOURCE;
                 c.name = "V" + std::to_string(++vCount);
                 c.value = 10.0;
                 hasSource = true;
             } else {
                 // Fallback to Resistor if max sources reached
                 c.type = RESISTOR;
                 c.name = "R" + std::to_string(++rCount);
                 c.value = 100.0 * (rand()%10 + 1);
             }
        } else if (typeRoll < 60) {
            // Resistor (High probability to ensure connectivity)
            c.type = RESISTOR;
            c.name = "R" + std::to_string(++rCount);
            c.value = 100.0 * (rand()%10 + 1);
        } else if (typeRoll < 85) {
             // Voltage Source
             if (vCount + iCount < 2) {
                 c.type = VOLTAGE_SOURCE;
                 c.name = "V" + std::to_string(++vCount);
                 c.value = 5.0 * (rand()%4 + 1);
                 hasSource = true;
             } else {
                 // Fallback
                 c.type = RESISTOR;
                 c.name = "R" + std::to_string(++rCount);
                 c.value = 100.0 * (rand()%10 + 1);
             }
        } else {
             // Current Source
             if (vCount + iCount < 2) {
                 c.type = CURRENT_SOURCE;
                 c.name = "I" + std::to_string(++iCount);
                 c.value = 1.0 * (rand()%5 + 1);
                 hasSource = true;
             } else {
                 // Fallback
                 c.type = RESISTOR;
                 c.name = "R" + std::to_string(++rCount);
                 c.value = 100.0 * (rand()%10 + 1);
             }
        }
        circuit.addComponent(c);
    }
    
    // Ensure we have at least one dynamic element and one switch
    bool foundDyn = false;
    bool foundSw = false;
    for(auto& c : circuit.components) {
        if(c.type == CAPACITOR || c.type == INDUCTOR) foundDyn = true;
        if(c.type == SWITCH) foundSw = true;
    }
    
    if (!foundDyn) {
        // Force replace last component
        Component& c = circuit.components.back();
        if (isRC) { c.type = CAPACITOR; c.name = "C1"; c.value = 100.0; }
        else { c.type = INDUCTOR; c.name = "L1"; c.value = 10.0; }
    }
    if (!foundSw && circuit.components.size() > 1) {
        // Force replace second to last (if not dyn)
        int idx = circuit.components.size() - 2;
        if (idx >= 0 && circuit.components[idx].type != CAPACITOR && circuit.components[idx].type != INDUCTOR) {
            circuit.components[idx].type = SWITCH;
            circuit.components[idx].name = "Sw1";
            circuit.components[idx].startsOpen = (rand()%2==0);
        }
    }
    
    // Generate question text (generic for transient)
    circuit.generateQuestion(); // Logic mostly unused now for interactiveness, but keeps Question Text in SVG?
    // Let's update generateQuestion to mention the transient nature.
    
    // We can just set the text manually here for the SVG.
    // "Calculate i(t)" or "Calculate v(t)"
    std::string var = isRC ? "v_C(t)" : "i_L(t)";
    circuit.questionText = "The switch changes state at t=0. Calculate the transient response " + var + ".";
    circuit.targetComp = nullptr; // Will be set by solver auto-detect? 
    // Actually solveTransient needs targetComp.
    // Let's find it.
    for(auto& c : circuit.components) {
        if(c.type == CAPACITOR || c.type == INDUCTOR) {
            circuit.targetComp = &c;
            break;
        }
    }

    return circuit;
}

// ========== Generate AC Steady State Circuit ==========
Circuit generateACCircuit() {
    int w = 3;
    int h = 3;
    Circuit circuit(w, h);
    circuit.exerciseType = AC_STEADY_STATE;
    
    // Track edges
    std::vector<std::pair<int, int>> edges;
    std::set<std::pair<int, int>> existingEdges;
    
    int numNodes = w * h;
    std::vector<int> visited = {0};
    std::vector<int> unvisited;
    for(int i=1; i<numNodes; ++i) unvisited.push_back(i);
    
    auto addEdge = [&](int u, int v) {
        if (u > v) std::swap(u, v);
        if (existingEdges.find({u, v}) == existingEdges.end()) {
            existingEdges.insert({u, v});
            edges.push_back({u, v});
        }
    };
    
    auto getNeighbors = [&](int u) {
        std::vector<int> n;
        int x = u % w;
        int y = u / w;
        if (x > 0) n.push_back(u - 1);
        if (x < w - 1) n.push_back(u + 1);
        if (y > 0) n.push_back(u - w);
        if (y < h - 1) n.push_back(u + w);
        return n;
    };
   
    // Spanning tree
    while (!unvisited.empty()) {
        std::vector<std::pair<int, int>> candidates;
        for (int u : visited) {
            for (int v : getNeighbors(u)) {
                bool isUnvisited = false;
                for(int uv : unvisited) if(uv == v) isUnvisited = true;
                if (isUnvisited) {
                    candidates.push_back({u, v});
                }
            }
        }
        if (candidates.empty()) break;
        int idx = rand() % candidates.size();
        std::pair<int, int> edge = candidates[idx];
        addEdge(edge.first, edge.second);
        visited.push_back(edge.second);
        for(size_t i=0; i<unvisited.size(); ++i) {
            if (unvisited[i] == edge.second) {
                unvisited.erase(unvisited.begin() + i);
                break;
            }
        }
    }
    
    // Add extra edges
    int cyclesToAdd = 4;
    for (int i=0; i<cyclesToAdd * 10; ++i) {
        if (cyclesToAdd <= 0) break;
        int u = rand() % numNodes;
        auto neighbors = getNeighbors(u);
        if (neighbors.empty()) continue;
        int v = neighbors[rand() % neighbors.size()];
        int u_s = std::min(u, v);
        int v_s = std::max(u, v);
        if (existingEdges.find({u_s, v_s}) == existingEdges.end()) {
            addEdge(u_s, v_s);
            cyclesToAdd--;
        }
    }
    
    // Assign components
    int rCount = 0, vCount = 0, iCount = 0, cCount = 0, lCount = 0;
    
    // Decide number of sources (1 or 2)
    int numSources = (rand() % 2 == 0) ? 1 : 2;
    int sourcesPlaced = 0;
    
    auto isPerimeter = [&](int n) {
        int x = n % w;
        int y = n / w;
        return x==0 || x==w-1 || y==0 || y==h-1;
    };
    
    std::random_shuffle(edges.begin(), edges.end());
    
    // Place at least 2 reactive components (L/C)
    int reactiveCount = 0;
    int reactiveTarget = 2 + rand() % 2; // 2-3 reactive components
    
    for (const auto& edge : edges) {
        Component c;
        c.nodeA_idx = edge.first;
        c.nodeB_idx = edge.second;
        c.pA = circuit.nodes[edge.first];
        c.pB = circuit.nodes[edge.second];
        
        bool perimeterEdge = isPerimeter(edge.first) && isPerimeter(edge.second);
        
        // Priority 1: Place reactive components
        if (reactiveCount < reactiveTarget && (rand() % 10 < 4)) {
            if (rand() % 2 == 0) {
                c.type = CAPACITOR;
                c.name = "C" + std::to_string(++cCount);
                c.value = 1.0 * (rand()%10 + 1); // µF
            } else {
                c.type = INDUCTOR;
                c.name = "L" + std::to_string(++lCount);
                c.value = 1.0 * (rand()%10 + 1); // mH
            }
            reactiveCount++;
            circuit.addComponent(c);
            continue;
        }
        
        // Priority 2: Sources
        if (sourcesPlaced < numSources && perimeterEdge) {
            if (rand() % 2 == 0) {
                c.type = VOLTAGE_SOURCE;
                c.name = "V" + std::to_string(++vCount);
                c.value = 5.0 * (rand()%4 + 1);
            } else {
                c.type = CURRENT_SOURCE;
                c.name = "I" + std::to_string(++iCount);
                c.value = 1.0 * (rand()%3 + 1);
            }
            sourcesPlaced++;
            circuit.addComponent(c);
            continue;
        }
        
        // Default: Resistors
        c.type = RESISTOR;
        c.name = "R" + std::to_string(++rCount);
        c.value = 10.0 * (rand()%10 + 1);
        circuit.addComponent(c);
    }
    
    // Ensure we have at least one source
    if (sourcesPlaced == 0 && !circuit.components.empty()) {
        Component& c = circuit.components[0];
        c.type = VOLTAGE_SOURCE;
        c.name = "V1";
        c.value = 10.0;
        sourcesPlaced = 1;
    }
    
    // Ensure we have at least 2 reactive components
    if (reactiveCount < 2) {
        for (size_t i = 0; i < circuit.components.size() && reactiveCount < 2; ++i) {
            if (circuit.components[i].type == RESISTOR) {
                if (rand() % 2 == 0) {
                    circuit.components[i].type = CAPACITOR;
                    circuit.components[i].name = "C" + std::to_string(++cCount);
                    circuit.components[i].value = 1.0 * (rand()%10 + 1);
                } else {
                    circuit.components[i].type = INDUCTOR;
                    circuit.components[i].name = "L" + std::to_string(++lCount);
                    circuit.components[i].value = 1.0 * (rand()%10 + 1);
                }
                reactiveCount++;
            }
        }
    }
    
    // Pick 1-3 resistors for power calculation
    std::vector<Component*> resistors;
    for (auto& c : circuit.components) {
        if (c.type == RESISTOR) resistors.push_back(&c);
    }
    
    if (!resistors.empty()) {
        int numPowerQuestions = std::min((int)resistors.size(), 1 + rand() % 3);
        std::random_shuffle(resistors.begin(), resistors.end());
        for (int i = 0; i < numPowerQuestions && i < (int)resistors.size(); ++i) {
            resistors[i]->highlightRed = true;
            circuit.targetResistors.push_back(resistors[i]);
        }
    }
    
    // ===== INTELLIGENT FREQUENCY SELECTION WITH ROUND VALUES =====
    int strategy = rand() % 10;
    
    // Possible round omega values (rad/s)
    std::vector<int> roundOmegas = {10, 50, 100, 200, 300, 500, 1000, 2000, 5000, 7000, 10000};
    
    if (strategy < 4 && cCount > 0 && lCount > 0) {
        // Strategy 1 (40%): Resonance - ω = 1/√(LC)
        Component* selectedL = nullptr;
        Component* selectedC = nullptr;
        for (auto& c : circuit.components) {
            if (!selectedL && c.type == INDUCTOR) selectedL = &c;
            if (!selectedC && c.type == CAPACITOR) selectedC = &c;
            if (selectedL && selectedC) break;
        }
        
        if (selectedL && selectedC) {
            double L_h = selectedL->value * 1e-3;
            double C_f = selectedC->value * 1e-6;
            double omega_resonance = 1.0 / std::sqrt(L_h * C_f);
            
            // Find closest round omega
            int closestOmega = roundOmegas[0];
            double minDiff = std::abs(omega_resonance - closestOmega);
            for (int omega : roundOmegas) {
                double diff = std::abs(omega_resonance - omega);
                if (diff < minDiff) {
                    minDiff = diff;
                    closestOmega = omega;
                }
            }
            circuit.omega = closestOmega;
            circuit.frequency = circuit.omega / (2.0 * 3.14159265359);
        } else {
            circuit.omega = 1000.0;
            circuit.frequency = circuit.omega / (2.0 * 3.14159265359);
        }
    } else if (strategy < 8) {
        // Strategy 2 (40%): Unit simplification - ω = 1000 rad/s
        circuit.omega = 1000.0;
        circuit.frequency = circuit.omega / (2.0 * 3.14159265359);
    } else {
        // Strategy 3 (20%): Random round value
        circuit.omega = roundOmegas[rand() % roundOmegas.size()];
        circuit.frequency = circuit.omega / (2.0 * 3.14159265359);
    }
    
    // Generate question text
    std::stringstream ss;
    
    // Randomly choose sin or cos
    circuit.sourceWaveform = (rand() % 2 == 0) ? "sin" : "cos";
    
    // Generate random amplitude (1-10 A)
    std::vector<int> possibleAmps = {1, 2, 3, 5, 10};
    circuit.sourceAmplitude = possibleAmps[rand() % possibleAmps.size()];
    
    // Show waveform with concrete amplitude value
    ss << "Circuito AC con i_s(t) = " << circuit.sourceAmplitude << "*" 
       << circuit.sourceWaveform << "(" << (int)circuit.omega << "t) A. ";
    
    if (!circuit.targetResistors.empty()) {
        ss << "Calcola la potenza media assorbita da ";
        for (size_t i = 0; i < circuit.targetResistors.size(); ++i) {
            ss << circuit.targetResistors[i]->name;
            if (i < circuit.targetResistors.size() - 1) ss << ", ";
        }
        ss << ". ";
    }
    
    if (sourcesPlaced == 1) {
       ss << "Calcola il fattore di potenza del generatore.";
    }
    
    circuit.questionText = ss.str();
    
    return circuit;
}


int main(int argc, char* argv[]) {
    srand(time(0));
    
    // Check for Headless Mode and Exercise Type
    bool headless = false;
    ExerciseType exerciseType = DC_TRANSIENT;
    
    if (argc > 1 && std::string(argv[1]) == "headless") {
        headless = true;
    }
    
    if (argc > 2) {
        std::string typeArg = argv[2];
        if (typeArg == "AC" || typeArg == "ac") {
            exerciseType = AC_STEADY_STATE;
        } else if (typeArg == "DC" || typeArg == "dc") {
            exerciseType = DC_TRANSIENT;
        }
    }
    
    // Generate appropriate circuit
    Circuit c = (exerciseType == AC_STEADY_STATE) ? generateACCircuit() : generateGridCircuit();
    
    if (!headless) {
        // Text Output
        if (exerciseType == AC_STEADY_STATE) {
            std::cout << "Generated AC Steady State Circuit." << std::endl;
        } else {
            std::cout << "Generated First Order Circuit with Switch." << std::endl;
        }
        for(const auto& comp : c.components) {
            std::cout << comp.toString() << std::endl;
        }
        std::cout << "\n[AI Tutor Question]: " << c.questionText << std::endl;
    }

    // Visual Output
    c.exportSVG("circuit.svg");
    
    // Solve and output
    if (exerciseType == AC_STEADY_STATE) {
        // AC Mode
        Circuit::ACResult result = c.solveAC();
        
        if (headless) {
            // JSON Output for GUI
            std::cout << "{" << std::endl;
            std::cout << "  \"exercise_type\": \"AC\"," << std::endl;
            std::cout << "  \"frequency\": " << c.frequency << "," << std::endl;
            std::cout << "  \"omega\": " << c.omega << "," << std::endl;
            std::cout << "  \"source_waveform\": \"" << c.sourceWaveform << "\"," << std::endl;
            std::cout << "  \"source_amplitude\": " << c.sourceAmplitude << "," << std::endl;
            std::cout << "  \"avg_power\": {" << std::endl;
            
            size_t count = 0;
            for (const auto& pair : result.avgPower) {
                std::cout << "    \"" << pair.first << "\": " << pair.second;
                if (count < result.avgPower.size() - 1) std::cout << ",";
                std::cout << std::endl;
                count++;
            }
            
            std::cout << "  }";
            if (result.hasPowerFactor) {
                std::cout << "," << std::endl;
                std::cout << "  \"power_factor\": " << result.powerFactor << std::endl;
            } else {
                std::cout << std::endl;
            }
            std::cout << "}" << std::endl;
            return 0;
        }
        
        // Interactive AC mode (terminal)
        std::cout << "\n=== AC Circuit Analysis ===" << std::endl;
        std::cout << "Frequency: " << c.frequency << " Hz" << std::endl;
        std::cout << "Omega: " << c.omega << " rad/s" << std::endl;
        
        for (const auto& pair : result.avgPower) {
            std::cout << "Average Power in " << pair.first << ": " << pair.second << " W" << std::endl;
        }
        
        if (result.hasPowerFactor) {
            std::cout << "Power Factor: " << result.powerFactor << std::endl;
        }
        
    } else {
        // DC Transient Mode
        Circuit::TransientResult result = c.solveTransient();
        std::string var = (c.targetComp && c.targetComp->type == INDUCTOR) ? "i_L" : "v_C";
        
        if (headless) {
            // JSON Output for GUI
            std::cout << "{" << std::endl;
            std::cout << "  \"exercise_type\": \"DC\"," << std::endl;
            std::cout << "  \"initial\": " << result.initialVal << "," << std::endl;
            std::cout << "  \"final\": " << result.finalVal << "," << std::endl;
            std::cout << "  \"tau\": " << result.tau << "," << std::endl;
            std::cout << "  \"variable\": \"" << var << "\"" << std::endl;
            std::cout << "}" << std::endl;
            return 0;
        }
        
        // Interactive Solver (Terminal)
        double userAnswer;
        
        auto ask = [&](std::string prompt, double correctVal) {
            std::cout << "\n> " << prompt << ": ";
            if (std::cin >> userAnswer) {
                 double diff = std::abs(userAnswer - correctVal);
                 double error = (std::abs(correctVal) > 1e-9) ? (diff / std::abs(correctVal)) : diff;
                 if (error < 0.05) {
                     std::cout << "Correct!" << std::endl;
                 } else {
                     std::cout << "Incorrect. The correct answer was: " << correctVal << std::endl;
                 }
            } else {
                std::cout << "Invalid input." << std::endl;
                exit(0);
            }
        };
        
        ask("Calculate " + var + "(0-) [Initial Value]", result.initialVal);
        ask("Calculate " + var + "(infinity) [Final Value]", result.finalVal);
        ask("Calculate Tau (Time Constant)", result.tau);
        
        std::cout << "\nThe full transient response is: " << var << "(t) = " 
                  << result.finalVal << " + (" << result.initialVal << " - " << result.finalVal << ") * e^(-t / " << result.tau << ")" << std::endl;
    }

    return 0;
}
