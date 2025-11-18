// Dictionnaire Morse
const morseCode = {
    'A': '.-', 'B': '-...', 'C': '-.-.', 'D': '-..', 'E': '.', 'F': '..-.',
    'G': '--.', 'H': '....', 'I': '..', 'J': '.---', 'K': '-.-', 'L': '.-..',
    'M': '--', 'N': '-.', 'O': '---', 'P': '.--.', 'Q': '--.-', 'R': '.-.',
    'S': '...', 'T': '-', 'U': '..-', 'V': '...-', 'W': '.--', 'X': '-..-',
    'Y': '-.--', 'Z': '--..', 
    '0': '-----', '1': '.----', '2': '..---', '3': '...--', '4': '....-',
    '5': '.....', '6': '-....', '7': '--...', '8': '---..', '9': '----.',
    '.': '.-.-.-', ',': '--..--', '?': '..--..', "'": '.----.', '!': '-.-.--',
    '/': '-..-.', '(': '-.--.', ')': '-.--.-', '&': '.-...', ':': '---...',
    ';': '-.-.-.', '=': '-...-', '+': '.-.-.', '-': '-....-', '_': '..--.-',
    '"': '.-..-.', '$': '...-..-', '@': '.--.-.', ' ': '/'
};

// Reverse dictionary pour décoder
const reverseMorse = {};
for (const [key, value] of Object.entries(morseCode)) {
    reverseMorse[value] = key;
}

// Variables globales
let mqttClient = null;
let currentMorseCode = '';
let isConnected = false;
let currentUser = null;

// URL du serveur backend
const API_BASE_URL = 'http://localhost:3000';

// Éléments DOM
const elements = {
    // Boutons d'authentification
    loginBtn: document.getElementById('loginBtn'),
    registerBtn: document.getElementById('registerBtn'),
    logoutBtn: document.getElementById('logoutBtn'),
    authSection: document.getElementById('authSection'),
    userSection: document.getElementById('userSection'),
    usernameDisplay: document.getElementById('usernameDisplay'),
    
    // Modals
    loginModal: document.getElementById('loginModal'),
    registerModal: document.getElementById('registerModal'),
    confirmLogin: document.getElementById('confirmLogin'),
    confirmRegister: document.getElementById('confirmRegister'),
    
    // MQTT
    mqttServer: document.getElementById('mqttServer'),
    deviceId: document.getElementById('deviceId'),
    connectBtn: document.getElementById('connectBtn'),
    mqttStatus: document.getElementById('mqttStatus'),
    
    // Morse
    dotBtn: document.getElementById('dotBtn'),
    dashBtn: document.getElementById('dashBtn'),
    spaceBtn: document.getElementById('spaceBtn'),
    validateBtn: document.getElementById('validateBtn'),
    clearBtn: document.getElementById('clearBtn'),
    morseCode: document.getElementById('morseCode'),
    translateBtn: document.getElementById('translateBtn'),
    translationResult: document.getElementById('translationResult'),
    
    // Messages
    receivedMessages: document.getElementById('receivedMessages')
};

// Fonctions API
async function registerUser(username, password, confirmPassword) {
    if (!username || !password) {
        throw new Error('Veuillez remplir tous les champs');
    }
    
    if (username.length < 3) {
        throw new Error('Le nom d\'utilisateur doit contenir au moins 3 caractères');
    }
    
    if (password.length < 6) {
        throw new Error('Le mot de passe doit contenir au moins 6 caractères');
    }
    
    if (password !== confirmPassword) {
        throw new Error('Les mots de passe ne correspondent pas');
    }

    try {
        const response = await fetch(`${API_BASE_URL}/register`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password }),
        });

        const data = await response.json();
        
        if (!data.success) {
            throw new Error(data.message);
        }
        
        return data;
    } catch (error) {
        if (error.message.includes('Failed to fetch')) {
            throw new Error('Impossible de se connecter au serveur. Vérifiez que le serveur est démarré.');
        }
        throw error;
    }
}

async function loginUser(username, password) {
    if (!username || !password) {
        throw new Error('Veuillez remplir tous les champs');
    }

    try {
        const response = await fetch(`${API_BASE_URL}/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password }),
        });

        const data = await response.json();
        
        if (!data.success) {
            throw new Error(data.message);
        }
        
        return data;
    } catch (error) {
        if (error.message.includes('Failed to fetch')) {
            throw new Error('Impossible de se connecter au serveur. Vérifiez que le serveur est démarré.');
        }
        throw error;
    }
}

// Gestion de l'authentification
function showLoginModal() {
    elements.loginModal.style.display = 'block';
    // Réinitialiser les champs
    document.getElementById('loginUsername').value = '';
    document.getElementById('loginPassword').value = '';
}

function showRegisterModal() {
    elements.registerModal.style.display = 'block';
    // Réinitialiser les champs
    document.getElementById('registerUsername').value = '';
    document.getElementById('registerPassword').value = '';
    document.getElementById('confirmPassword').value = '';
}

function hideModals() {
    elements.loginModal.style.display = 'none';
    elements.registerModal.style.display = 'none';
}

async function login(username, password) {
    try {
        elements.confirmLogin.disabled = true;
        elements.confirmLogin.textContent = 'Connexion...';
        
        const result = await loginUser(username, password);
        currentUser = result.username;
        localStorage.setItem('currentUser', currentUser);
        updateAuthUI();
        hideModals();
        addMessage('Connexion réussie!', 'success');
    } catch (error) {
        addMessage(error.message, 'error');
    } finally {
        elements.confirmLogin.disabled = false;
        elements.confirmLogin.textContent = 'Se connecter';
    }
}

async function register(username, password, confirmPassword) {
    try {
        elements.confirmRegister.disabled = true;
        elements.confirmRegister.textContent = 'Inscription...';
        
        await registerUser(username, password, confirmPassword);
        addMessage('Inscription réussie! Vous pouvez maintenant vous connecter.', 'success');
        hideModals();
        // Afficher automatiquement le modal de connexion après inscription
        setTimeout(() => showLoginModal(), 1000);
    } catch (error) {
        addMessage(error.message, 'error');
    } finally {
        elements.confirmRegister.disabled = false;
        elements.confirmRegister.textContent = 'S\'inscrire';
    }
}

function logout() {
    currentUser = null;
    localStorage.removeItem('currentUser');
    updateAuthUI();
    disconnectMQTT();
    addMessage('Déconnexion réussie', 'success');
}

function updateAuthUI() {
    if (currentUser) {
        elements.authSection.style.display = 'none';
        elements.userSection.style.display = 'flex';
        elements.usernameDisplay.textContent = currentUser;
    } else {
        elements.authSection.style.display = 'flex';
        elements.userSection.style.display = 'none';
    }
}

// Le reste du code reste inchangé...
// [Gestion MQTT, gestion du code Morse, etc. - conservez tout le code existant à partir d'ici]

// Gestion MQTT
function connectMQTT() {
    if (!currentUser) {
        addMessage('Veuillez vous connecter d\'abord', 'error');
        return;
    }
    
    const server = elements.mqttServer.value;
    const deviceId = elements.deviceId.value || 'ESP32_MORSE_001';
    
    if (!server) {
        addMessage('Veuillez spécifier un serveur MQTT', 'error');
        return;
    }
    
    elements.mqttStatus.textContent = 'Connexion...';
    elements.mqttStatus.className = 'status connecting';
    elements.connectBtn.disabled = true;
    
    try {
        mqttClient = mqtt.connect(server, {
            clientId: `morse_web_${currentUser}_${Math.random().toString(16).slice(2)}`,
            clean: true,
            reconnectPeriod: 5000,  // ⚠️ IMPORTANT - reconnexion auto
            connectTimeout: 10000   // ⚠️ IMPORTANT - timeout de connexion
        });
        
        mqttClient.on('connect', () => {
            isConnected = true;
            elements.mqttStatus.textContent = 'Connecté';
            elements.mqttStatus.className = 'status connected';
            elements.connectBtn.textContent = 'Déconnecter';
            elements.connectBtn.disabled = false;
            
            // S'abonner aux topics
            mqttClient.subscribe(`morse/${deviceId}/from_device`);
            mqttClient.subscribe(`morse/${deviceId}/status`);
            
            addMessage(`Connecté au serveur MQTT: ${server}`, 'success');
            addMessage(`Abonné aux topics: morse/${deviceId}/from_device et morse/${deviceId}/status`, 'success');
        });
        
        mqttClient.on('message', (topic, message) => {
            const msg = message.toString();
            addMessage(`Message reçu [${topic}]: ${msg}`, 'success');
            
            if (topic === `morse/${deviceId}/from_device`) {
                handleDeviceMessage(msg);
            }
        });
        
        mqttClient.on('error', (error) => {
            addMessage(`Erreur MQTT: ${error.message}`, 'error');
            elements.mqttStatus.textContent = 'Erreur de connexion';
            elements.mqttStatus.className = 'status disconnected';
            elements.connectBtn.disabled = false;
        });
        
        mqttClient.on('close', () => {
            if (isConnected) {
                addMessage('Déconnecté du serveur MQTT', 'error');
            }
            isConnected = false;
            elements.mqttStatus.textContent = 'Déconnecté';
            elements.mqttStatus.className = 'status disconnected';
            elements.connectBtn.textContent = 'Se connecter à MQTT';
            elements.connectBtn.disabled = false;
        });
        
    } catch (error) {
        addMessage(`Erreur de connexion: ${error.message}`, 'error');
        elements.mqttStatus.textContent = 'Erreur';
        elements.mqttStatus.className = 'status disconnected';
        elements.connectBtn.disabled = false;
    }
}

function disconnectMQTT() {
    if (mqttClient) {
        mqttClient.end();
        mqttClient = null;
    }
    isConnected = false;
}

function toggleMQTTConnection() {
    if (isConnected) {
        disconnectMQTT();
    } else {
        connectMQTT();
    }
}

function sendMorseCommand(command) {
    if (!isConnected || !mqttClient) {
        addMessage('Non connecté à MQTT', 'error');
        return;
    }
    
    const deviceId = elements.deviceId.value || 'ESP32_MORSE_001';
    mqttClient.publish(`morse/${deviceId}/from_web`, command);
    addMessage(`Commande envoyée: ${command}`, 'success');
}

function handleDeviceMessage(message) {
    // Traiter les messages de l'ESP32
    if (message.startsWith('CHAR:')) {
        const char = message.substring(5);
        elements.translationResult.value = char;
    } else if (message.startsWith('MORSE:')) {
        const morse = message.substring(6);
        elements.morseCode.value = morse;
    }
}

// Gestion du code Morse
function addDot() {
    currentMorseCode += '.';
    elements.morseCode.value = currentMorseCode;
    sendMorseCommand('DOT');
}

function addDash() {
    currentMorseCode += '-';
    elements.morseCode.value = currentMorseCode;
    sendMorseCommand('DASH');
}

function addCharacterSeparator() {
    if (currentMorseCode !== '' && !currentMorseCode.endsWith('/')) {
        currentMorseCode += '/';
        elements.morseCode.value = currentMorseCode;
    }
}

function addSpaceOnly() {
    currentMorseCode += ' ';
    elements.morseCode.value = currentMorseCode;
    sendMorseCommand('SPACE');
    addMessage('Espace ajouté', 'success');
}

function validateCharacter() {
    if (currentMorseCode.trim() === '') {
        addMessage('Aucun code Morse à valider', 'error');
        return;
    }
    
    // Ajouter le séparateur de caractère s'il n'y en a pas déjà
    if (currentMorseCode !== '' && !currentMorseCode.endsWith('/') && !currentMorseCode.endsWith(' ')) {
        currentMorseCode += '/';
        elements.morseCode.value = currentMorseCode;
    }
    
    // Traduire le caractère actuel
    translateMorse();
}

function translateMorse() {
    if (currentMorseCode.trim() === '') {
        addMessage('Aucun code Morse à traduire', 'error');
        return;
    }
    
    try {
        let result = '';
        
        // Nettoyer le code morse des séparateurs en double
        let cleanMorse = currentMorseCode.replace(/\/+/g, '/').replace(/\/ $/, ' ');
        
        // Séparer les mots (espaces) et les caractères (/)
        const words = cleanMorse.split(' ');
        
        for (const word of words) {
            if (word === '') {
                result += ' ';
                continue;
            }
            
            const characters = word.split('/').filter(char => char !== '');
            for (const char of characters) {
                if (reverseMorse[char]) {
                    result += reverseMorse[char];
                } else if (char !== '') {
                    result += '?'; // Caractère inconnu
                }
            }
            result += ' ';
        }
        
        elements.translationResult.value = result.trim();
        addMessage(`Traduction: ${result.trim()}`, 'success');
        
        // Envoyer la traduction à l'ESP32
        sendMorseCommand(`TRANSLATION:${result.trim()}`);
        
        // Réinitialiser pour le prochain caractère
        currentMorseCode = '';
        elements.morseCode.value = '';
        
    } catch (error) {
        addMessage(`Erreur de traduction: ${error.message}`, 'error');
    }
}

function clearMorse() {
    currentMorseCode = '';
    elements.morseCode.value = '';
    elements.translationResult.value = '';
    addMessage('Code Morse effacé', 'success');
}

function handleBackspace() {
    if (currentMorseCode.length > 0) {
        currentMorseCode = currentMorseCode.slice(0, -1);
        elements.morseCode.value = currentMorseCode;
    }
}

function addMessage(text, type = '') {
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${type}`;
    messageDiv.textContent = `[${new Date().toLocaleTimeString()}] ${text}`;
    
    elements.receivedMessages.appendChild(messageDiv);
    elements.receivedMessages.scrollTop = elements.receivedMessages.scrollHeight;
    
    // Limiter à 50 messages
    const messages = elements.receivedMessages.getElementsByClassName('message');
    if (messages.length > 50) {
        messages[0].remove();
    }
}

// Événements
document.addEventListener('DOMContentLoaded', () => {
    // Vérifier si un utilisateur est déjà connecté
    const savedUser = localStorage.getItem('currentUser');
    if (savedUser) {
        currentUser = savedUser;
        updateAuthUI();
    }
    
    // Authentification
    elements.loginBtn.addEventListener('click', showLoginModal);
    elements.registerBtn.addEventListener('click', showRegisterModal);
    elements.logoutBtn.addEventListener('click', logout);
    
    // Modals
    elements.confirmLogin.addEventListener('click', () => {
        const username = document.getElementById('loginUsername').value;
        const password = document.getElementById('loginPassword').value;
        login(username, password);
    });
    
    elements.confirmRegister.addEventListener('click', () => {
        const username = document.getElementById('registerUsername').value;
        const password = document.getElementById('registerPassword').value;
        const confirmPassword = document.getElementById('confirmPassword').value;
        register(username, password, confirmPassword);
    });
    
    // Fermer les modals
    document.querySelectorAll('.close').forEach(closeBtn => {
        closeBtn.addEventListener('click', hideModals);
    });
    
    window.addEventListener('click', (event) => {
        if (event.target.classList.contains('modal')) {
            hideModals();
        }
    });
    
    // MQTT
    elements.connectBtn.addEventListener('click', toggleMQTTConnection);
    
    // Morse
    elements.dotBtn.addEventListener('click', addDot);
    elements.dashBtn.addEventListener('click', addDash);
    elements.spaceBtn.addEventListener('click', addSpaceOnly);
    elements.validateBtn.addEventListener('click', validateCharacter);
    elements.clearBtn.addEventListener('click', clearMorse);
    elements.translateBtn.addEventListener('click', translateMorse);
    
    // Raccourcis clavier
    document.addEventListener('keydown', (event) => {
        if (!isConnected) return;
        
        switch(event.key) {
            case 'ArrowLeft':
                event.preventDefault();
                addDot();
                break;
            case 'ArrowRight':
                event.preventDefault();
                addDash();
                break;
            case ' ':
                event.preventDefault();
                addSpaceOnly();
                break;
            case 'Enter':
                event.preventDefault();
                validateCharacter();
                break;
            case 'Escape':
            case 'Backspace':
                event.preventDefault();
                clearMorse();
                break;
            case '/':
                event.preventDefault();
                addCharacterSeparator();
                break;
            case 'v':
            case 'V':
                if (event.ctrlKey) {
                    event.preventDefault();
                    validateCharacter();
                }
                break;
        }
    });
    
    addMessage('Application chargée. Veuillez vous connecter.');
});