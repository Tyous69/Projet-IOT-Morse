const express = require('express');
const fs = require('fs');
const path = require('path');
const bcrypt = require('bcryptjs');
const cors = require('cors');

const app = express();
const PORT = 3000;

// Middleware
app.use(express.json());
app.use(cors());
app.use(express.static('.')); // Servir les fichiers statiques

// Chemin vers le fichier JSON des utilisateurs
const USERS_FILE = path.join(__dirname, 'users.json');

// Initialiser le fichier users.json s'il n'existe pas
function initializeUsersFile() {
    if (!fs.existsSync(USERS_FILE)) {
        fs.writeFileSync(USERS_FILE, JSON.stringify({}));
    }
}

// Lire les utilisateurs
function readUsers() {
    try {
        const data = fs.readFileSync(USERS_FILE, 'utf8');
        return JSON.parse(data);
    } catch (error) {
        return {};
    }
}

// Écrire les utilisateurs
function writeUsers(users) {
    fs.writeFileSync(USERS_FILE, JSON.stringify(users, null, 2));
}

// Route d'inscription
app.post('/register', async (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ 
            success: false, 
            message: 'Nom d\'utilisateur et mot de passe requis' 
        });
    }

    if (username.length < 3) {
        return res.status(400).json({ 
            success: false, 
            message: 'Le nom d\'utilisateur doit contenir au moins 3 caractères' 
        });
    }

    if (password.length < 6) {
        return res.status(400).json({ 
            success: false, 
            message: 'Le mot de passe doit contenir au moins 6 caractères' 
        });
    }

    const users = readUsers();

    if (users[username]) {
        return res.status(400).json({ 
            success: false, 
            message: 'Cet utilisateur existe déjà' 
        });
    }

    try {
        // Hasher le mot de passe
        const hashedPassword = await bcrypt.hash(password, 10);
        
        // Sauvegarder l'utilisateur
        users[username] = {
            password: hashedPassword,
            createdAt: new Date().toISOString()
        };
        
        writeUsers(users);
        
        res.json({ 
            success: true, 
            message: 'Inscription réussie!' 
        });
    } catch (error) {
        res.status(500).json({ 
            success: false, 
            message: 'Erreur lors de l\'inscription' 
        });
    }
});

// Route de connexion
app.post('/login', async (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ 
            success: false, 
            message: 'Nom d\'utilisateur et mot de passe requis' 
        });
    }

    const users = readUsers();

    if (!users[username]) {
        return res.status(400).json({ 
            success: false, 
            message: 'Utilisateur non trouvé' 
        });
    }

    try {
        const isPasswordValid = await bcrypt.compare(password, users[username].password);
        
        if (isPasswordValid) {
            res.json({ 
                success: true, 
                message: 'Connexion réussie!',
                username: username
            });
        } else {
            res.status(400).json({ 
                success: false, 
                message: 'Mot de passe incorrect' 
            });
        }
    } catch (error) {
        res.status(500).json({ 
            success: false, 
            message: 'Erreur lors de la connexion' 
        });
    }
});

// Route pour vérifier si un utilisateur existe
app.get('/check-user/:username', (req, res) => {
    const { username } = req.params;
    const users = readUsers();
    
    res.json({ 
        exists: !!users[username] 
    });
});

// Route pour obtenir la liste des utilisateurs (pour debug)
app.get('/users', (req, res) => {
    const users = readUsers();
    res.json(users);
});

// Démarrer le serveur
app.listen(PORT, () => {
    initializeUsersFile();
    console.log(`Serveur démarré sur http://localhost:${PORT}`);
});