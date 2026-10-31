# Secure Vault (x86-64 NASM)

Gestionnaire de mots de passe en assembleur x86-64 pour Linux, réalisé sans la bibliothèque C standard (appels système directs via `syscall`).

Projet réalisé dans le cadre du module d'Assembleur et Reverse Engineering (M2).

## Fonctionnalités

* **F0 - Initialisation de la base :** Saisie du mot de passe maître, dérivation d'une clé de 8 octets et création/ouverture du fichier de stockage `vault.db`.
* **F1 - Ajout d'identifiant :** Saisie d'une paire identifiant / mot de passe (format fixe de 64 octets), déchiffrement en RAM du contenu existant, ajout de la nouvelle entrée et rechiffrement sur disque.
* **F2 - Consultation :** Lecture du fichier `vault.db`, vérification de la taille (multiple strict de 64 octets), déchiffrement en mémoire et affichage formaté.

## Format de stockage et chiffrement

* Fichier binaire plat `vault.db`.
* Chaque enregistrement correspond à une structure fixe de 64 octets :
  * `login` : 32 octets (chaîne ASCII terminée par `\0`, complétée par des octets `0x00`).
  * `password` : 32 octets (chaîne ASCII terminée par `\0`, complétée par des octets `0x00`).
* Chiffrement symétrique par blocs de 8 octets en mode XOR ECB cyclique.
* Intégrité minimale vérifiée via `test rax, 63` (la taille du fichier doit obligatoirement être un multiple de 64).

## Compilation

Prérequis :
* Linux x86-64
* `nasm`
* `ld` (GNU binutils)
* `make` (optionnel)

### Via Make

```bash
make
```

Pour nettoyer les fichiers temporaires et les binaires :
```bash
make clean
```

### Via le script shell

```bash
./build_nasm.sh secure_vault.asm
```

## Utilisation

Lancer l'exécutable :
```bash
./secure_vault
```

Le programme demande le mot de passe maître au démarrage puis affiche le menu interactif :
```text
========================================
     SECURE VAULT - GESTIONNAIRE     
========================================
Entrez le mot de passe maitre de la base : 

--- MENU PRINCIPAL ---
1. Ajouter un identifiant (F1)
2. Afficher les identifiants dechiffres (F2)
3. Quitter
Votre choix [1-3] :
```

## Structure du dépôt

* `secure_vault.asm` : Code source principal (menu, logique F0/F1/F2, structures mémoire).
* `inc/cours.inc` : Routines d'entrées/sorties bas niveau (lecture/écriture terminal, I/O fichiers, routine de chiffrement XOR).
* `Makefile` : Règle de compilation par défaut.
* `build_nasm.sh` : Script de compilation alternatif.

## Limitations de sécurité

Ce projet a une vocation pédagogique pour l'apprentissage de l'assembleur x86-64 et comporte des faiblesses cryptographiques connues :
* Le chiffrement XOR ECB ne masque pas les octets nuls (`0x00`) utilisés pour le bourrage des structures, ce qui expose la clé maîtresse dans les zones non remplies.
* Absence de code d'authentification de message (MAC / AEAD) permettant de détecter les altérations de données ou une saisie erronée de mot de passe maître lors d'un ajout.
* Absence d'étirement de clé (KDF salé type Argon2 ou PBKDF2).
