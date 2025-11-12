Commandes PlatformIO utiles: 
(il faut donc l'extension platformIO IDE sur vscode)
```
# Compiler le projet
pio run

# Téléverser sur l'ESP32
pio run --target upload

# Monitorer le port série
pio device monitor

# Nettoyer le projet
pio run --target clean
```

Commandes pour lancer la platforme web:
```
cd projet-morse-iot/web
python -m http.server 8000
```