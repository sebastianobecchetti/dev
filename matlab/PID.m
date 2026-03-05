function esperimento_pid_ode()
    % =====================================================================
    % 1. AREA ESPERIMENTI: MODIFICA QUESTI PARAMETRI!
    % =====================================================================
    Kp = 0;  % Guadagno Proporzionale (forza di reazione immediata)
    Ki = 0.0;  % Guadagno Integrale (azzera l'errore nel tempo)
    Kd = 0;   % Guadagno Derivativo (frena le oscillazioni)
    
    % IL FILTRO SALVAVITA (tau)
    % Valore corretto: 0.05 (filtra il rumore a 100 rad/s)
    % Prova a metterlo a 0.0005 per vedere il "chattering" distruttivo!
    tau = 0.05; 
    
    % =====================================================================
    % 2. IMPOSTAZIONI SIMULAZIONE
    % =====================================================================
    tspan = [0 15]; % Simuliamo per 15 secondi
    x0 = [0; 0; 0]; % Condizioni iniziali: [y(0), Integrale(0), Filtro(0)]
    
    % Usiamo ode15s perché il sistema ha frequenze miste (1 rad/s e 100 rad/s)
    options = odeset('RelTol', 1e-4, 'AbsTol', 1e-5);
    
    disp('Simulazione in corso... (se tau è minuscolo, ci vorrà più tempo!)');
    [t, x] = ode15s(@(t, x) equazioni_sistema(t, x, Kp, Ki, Kd, tau), tspan, x0, options);
    
    % =====================================================================
    % 3. ESTRAZIONE DATI E CALCOLO DEL SEGNALE DI CONTROLLO u(t)
    % =====================================================================
    y_uscita = x(:, 1);  % La prima colonna è l'uscita del sistema fisico
    riferimento = sin(t) + 0.01 * sin(100 * t); % Il tuo segnale
    
    % Ricalcoliamo u(t) per poterlo graficare (per vedere lo sforzo dell'attuatore)
    u_controllo = zeros(length(t), 1);
    for i = 1:length(t)
        errore = riferimento(i) - x(i, 1);
        u_P = Kp * errore;
        u_I = Ki * x(i, 2);
        u_D = (Kd / tau) * (errore - x(i, 3));
        u_controllo(i) = u_P + u_I + u_D;
    end

    % =====================================================================
    % 4. GRAFICI
    % =====================================================================
    figure('Name', 'Laboratorio PID', 'Position', [100, 100, 800, 600]);
    
    % Grafico 1: Inseguimento del segnale (Tracking)
    subplot(2, 1, 1);
    plot(t, riferimento, 'Color', [0.6 0.6 0.6], 'LineWidth', 1.5); hold on;
    plot(t, y_uscita, '-b', 'LineWidth', 1.5);
    title(sprintf('Risposta del Sistema (Kp=%g, Ki=%g, Kd=%g, \\tau=%g)', Kp, Ki, Kd, tau));
    ylabel('Ampiezza y(t)');
    legend('Segnale desiderato (ym)', 'Uscita reale del sistema', 'Location', 'best');
    grid on;
    
    % Grafico 2: Sforzo di controllo u(t) - L'energia inviata all'attuatore
    subplot(2, 1, 2);
    plot(t, u_controllo, '-r', 'LineWidth', 1.2);
    title('Sforzo di Controllo u(t) (Energia richiesta all''attuatore)');
    xlabel('Tempo [s]');
    ylabel('Ampiezza u(t)');
    grid on;
end

% =========================================================================
% FUNZIONE DELLE EQUAZIONI DIFFERENZIALI
% =========================================================================
function dxdt = equazioni_sistema(t, x, Kp, Ki, Kd, tau)
    % Estrazione stati
    y      = x(1); % Uscita del sistema
    int_e  = x(2); % Memoria dell'integrale
    filt_d = x(3); % Stato del filtro derivativo
    
    % Calcolo del segnale di riferimento in questo istante 't'
    r = sin(t) + 0.01 * sin(100 * t);
    errore = r - y;
    
    % Equazioni del PID
    u_P = Kp * errore;
    u_I = Ki * int_e;
    u_D = (Kd / tau) * (errore - filt_d);
    
    u = u_P + u_I + u_D; % Sforzo totale
    
    % Inizializzo il vettore colonna delle derivate
    dxdt = zeros(3,1);
    
    % 1. Modello del sistema fisico da controllare (Motore 1° ordine: y' = -5y + 5u)
    dxdt(1) = -5 * y + 5 * u; 
    
    % 2. Dinamica dell'integratore (derivata dell'integrale = errore)
    dxdt(2) = errore;
    
    % 3. Dinamica del filtro sulla derivata
    dxdt(3) = (errore - filt_d) / tau;
end