; ==============================================================================
; M2 REVERSE ENGINEERING & ASSEMBLEUR x86-64 — EXAMEN FINAL : SECURE VAULT
; ==============================================================================
; Architecture : x86-64 Linux (Syntaxe Intel NASM)
; Chaîne       : nasm -f elf64 + ld (Pure Syscalls, zéro dépendance libc)
; Modularité   : Primitives d'E/S, mémoire et fichiers fournies par inc/cours.inc
; Barème       : F0 (2pts), F1 (2pts), F2 (2pts), Pile (4pts), Modularité (4pts)
; ==============================================================================

global _start

default rel

%include "./inc/cours.inc"

; ------------------------------------------------------------------------------
; Constantes & Structures de Données
; ------------------------------------------------------------------------------
CRED_FIELD_LEN  equ 32

struc credential
    .login:     resb CRED_FIELD_LEN     ; 32 octets (31 utiles + \0)
    .password:  resb CRED_FIELD_LEN     ; 32 octets (31 utiles + \0)
endstruc                                ; credential_size = 64 octets (multiple de 8)

MAX_RECORDS     equ 64
DB_MAX_SIZE     equ (MAX_RECORDS * credential_size) ; 4096 octets
KEY_LEN         equ 8                   ; Chiffrement XOR ECB par bloc de 8 octets

; ==============================================================================
; Section .text : Code Exécutable
; ==============================================================================
section .text

; ------------------------------------------------------------------------------
; Point d'entrée principal : _start
; ------------------------------------------------------------------------------
_start:
    ; Bannière d'accueil
    mov rdi, msg_banner
    call print

    ; 1. Initialisation F0 (Mot de passe maître & base)
    call f0_init
    test rax, rax
    js .fatal_error

.main_loop:
    ; 2. Affichage du Menu Principal
    mov rdi, msg_menu
    call print

    ; Saisie du choix utilisateur (1 caractère + Entrée)
    mov rdi, input_buf
    mov rsi, 8
    call read_line_bounded

    ; Analyse du choix
    mov al, byte [input_buf]
    cmp al, '1'
    je .do_f1
    cmp al, '2'
    je .do_f2
    cmp al, '3'
    je .do_quit

    ; Choix invalide
    mov rdi, msg_bad_choice
    call print
    jmp .main_loop

.do_f1:
    call f1_add
    jmp .main_loop

.do_f2:
    call f2_display
    jmp .main_loop

.do_quit:
    mov rdi, msg_quit
    call print
    xor rdi, rdi            ; code retour 0
    mov rax, 60             ; sys_exit
    syscall

.fatal_error:
    mov rdi, 1              ; code retour 1
    mov rax, 60             ; sys_exit
    syscall

; ------------------------------------------------------------------------------
; F0 : Initialisation, Dérivation de la clé maître et Création de la base
; Signature : f0_init() -> rax (0 = succès, <0 = erreur)
; ------------------------------------------------------------------------------
f0_init:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    ; Nettoie le tampon de la clé (8 octets à zéro)
    mov rdi, master_key
    xor sil, sil
    mov rdx, KEY_LEN
    call memset

    ; Invite de saisie du mot de passe maître
    mov rdi, msg_f0_prompt
    call print

    ; Lecture bornée du mot de passe maître (jusqu'à 64 caractères)
    mov rdi, input_buf
    mov rsi, 64
    call read_line_bounded
    mov r12, rax            ; r12 = longueur du mot de passe saisi

    test r12, r12
    jz .f0_empty_pwd

    ; Dérivation de la clé XOR 8 octets (ECB) :
    ; On copie au maximum 8 octets du mot de passe dans master_key
    mov rdx, 8
    cmp r12, 8
    jbe .f0_copy_key
    mov r12, 8              ; bride à 8 octets si mot de passe plus long

.f0_copy_key:
    mov rdi, master_key
    mov rsi, input_buf
    mov rdx, r12
    call memcpy
    jmp .f0_touch_file

.f0_empty_pwd:
    ; Si mot de passe vide, on utilise une clé par défaut (8 octets nuls)
    mov rdi, msg_f0_warn_empty
    call print

.f0_touch_file:
    ; Ouverture / Création préventive du fichier "vault.db" sans écraser (0644)
    mov rdi, db_filename
    call open_file
    test rax, rax
    js .f0_err

    ; On referme immédiatement le descripteur (gestion sans état résiduel)
    mov rdi, rax
    call close_file

    mov rdi, msg_f0_ok
    call print

    xor rax, rax            ; Succès (0)
    jmp .f0_exit

.f0_err:
    mov rdi, msg_f0_err
    call print
    mov rax, -1

.f0_exit:
    pop r13
    pop r12
    pop rbx
    leave
    ret

; ------------------------------------------------------------------------------
; F1 : Ajout d'un Credential (Login + Password)
; 1. Saisie Login & Password (structures 32 octets)
; 2. Lecture et Déchiffrement de la base existante
; 3. Append du nouveau compte (64 octets)
; 4. Rechiffrement global et écriture disque
; Signature : f1_add() -> rax (0 = succès, <0 = erreur)
; ------------------------------------------------------------------------------
f1_add:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov rdi, msg_f1_title
    call print

    ; 1. Initialisation à zéro de la structure temporaire (64 octets)
    mov rdi, temp_cred
    xor sil, sil
    mov rdx, credential_size
    call memset

    ; 2. Saisie du Login (au plus 31 caractères + \0 automatique)
    mov rdi, msg_f1_login
    call print
    mov rdi, temp_cred + credential.login
    mov rsi, CRED_FIELD_LEN
    call read_line_bounded

    ; 3. Saisie du Password (au plus 31 caractères + \0 automatique)
    mov rdi, msg_f1_pass
    call print
    mov rdi, temp_cred + credential.password
    mov rsi, CRED_FIELD_LEN
    call read_line_bounded

    ; 4. Lecture de la base existante depuis vault.db
    mov rdi, db_filename
    mov rsi, db_buffer
    mov rdx, DB_MAX_SIZE
    call file_read_all
    test rax, rax
    js .f1_read_err
    mov r12, rax            ; r12 = taille actuelle en octets (N * 64)

    ; Vérification de la capacité maximale
    lea rax, [r12 + credential_size]
    cmp rax, DB_MAX_SIZE
    ja .f1_full_err

    ; 5. Si la base existante n'est pas vide, on déchiffre le tampon existant
    test r12, r12
    jz .f1_append_record

    ; Vérification d'intégrité (multiple de 64)
    test r12, 63
    jnz .f1_corrupt_err

    ; Déchiffrement in-place de la base existante avec la clé maître
    mov rdi, db_buffer
    mov rsi, r12
    mov rdx, master_key
    call xor_ecb8

.f1_append_record:
    ; 6. Copie des 64 octets de temp_cred à la suite de db_buffer (offset r12)
    lea rdi, [db_buffer + r12]
    mov rsi, temp_cred
    mov rdx, credential_size
    call memcpy

    ; Nouvelle taille totale
    add r12, credential_size

    ; 7. Rechiffrement in-place de l'intégralité du tampon (r12 octets)
    mov rdi, db_buffer
    mov rsi, r12
    mov rdx, master_key
    call xor_ecb8

    ; 8. Écriture atomique et synchronisée sur disque (création / remplacement)
    mov rdi, db_filename
    mov rsi, db_buffer
    mov rdx, r12
    call file_create_write
    test rax, rax
    js .f1_write_err

    mov rdi, msg_f1_ok
    call print
    xor rax, rax
    jmp .f1_exit

.f1_read_err:
    mov rdi, msg_err_read
    call print
    mov rax, -1
    jmp .f1_exit

.f1_full_err:
    mov rdi, msg_f1_full
    call print
    mov rax, -2
    jmp .f1_exit

.f1_corrupt_err:
    mov rdi, msg_err_corrupt
    call print
    mov rax, -3
    jmp .f1_exit

.f1_write_err:
    mov rdi, msg_err_write
    call print
    mov rax, -4

.f1_exit:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret

; ------------------------------------------------------------------------------
; F2 : Relecture, Déchiffrement et Affichage Formaté
; Signature : f2_display() -> rax (0 = succès, <0 = erreur)
; ------------------------------------------------------------------------------
f2_display:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov rdi, msg_f2_title
    call print

    ; 1. Lecture complète du fichier vault.db
    mov rdi, db_filename
    mov rsi, db_buffer
    mov rdx, DB_MAX_SIZE
    call file_read_all
    test rax, rax
    js .f2_read_err

    ; Si la taille est 0, la base est vide
    test rax, rax
    jz .f2_empty

    ; Vérification d'intégrité binaire (test rax, 63 -> rax % 64 == 0)
    test rax, 63
    jnz .f2_corrupt

    mov r12, rax            ; r12 = taille totale en octets

    ; 2. Déchiffrement in-place en RAM avec la clé maître
    mov rdi, db_buffer
    mov rsi, r12
    mov rdx, master_key
    call xor_ecb8

    ; 3. Boucle d'affichage de chaque enregistrement
    xor r13, r13            ; r13 = offset courant dans db_buffer (0, 64, 128...)
    xor r14, r14            ; r14 = numéro d'index (1, 2, 3...)

.f2_loop:
    cmp r13, r12
    jae .f2_done

    inc r14                 ; Compte #N

    ; Séparateur visuel
    mov rdi, msg_sep
    call print

    ; Affiche "Compte #"
    mov rdi, msg_entry_num
    call print
    mov rdi, r14
    call put_dec            ; Affiche le numéro décimal

    mov rdi, msg_newline
    call print

    ; Affiche "  Login    : "
    mov rdi, msg_lbl_login
    call print
    lea rdi, [db_buffer + r13 + credential.login]
    mov rsi, CRED_FIELD_LEN
    call print_cstr_n       ; Affichage sécurisé borné à 32 octets max (anti-crash)
    mov rdi, msg_newline
    call print

    ; Affiche "  Password : "
    mov rdi, msg_lbl_pass
    call print
    lea rdi, [db_buffer + r13 + credential.password]
    mov rsi, CRED_FIELD_LEN
    call print_cstr_n       ; Affichage sécurisé borné à 32 octets max
    mov rdi, msg_newline
    call print

    add r13, credential_size ; Avance de 64 octets
    jmp .f2_loop

.f2_empty:
    mov rdi, msg_f2_empty
    call print
    xor rax, rax
    jmp .f2_exit

.f2_corrupt:
    mov rdi, msg_err_corrupt
    call print
    mov rax, -2
    jmp .f2_exit

.f2_read_err:
    mov rdi, msg_err_read
    call print
    mov rax, -1
    jmp .f2_exit

.f2_done:
    mov rdi, msg_sep
    call print
    xor rax, rax

.f2_exit:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret

; ------------------------------------------------------------------------------
; Helper didactique : Affichage d'un entier en décimal (put_dec)
; Signature : put_dec(long rdi)
; ------------------------------------------------------------------------------
put_dec:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov rax, rdi
    mov rbx, 10
    xor rcx, rcx            ; compteur de chiffres empilés

.pd_div_loop:
    xor rdx, rdx
    div rbx                 ; rax = quotient, rdx = reste (chiffre entre 0 et 9)
    push rdx                ; empile le chiffre
    inc rcx
    test rax, rax
    jnz .pd_div_loop

.pd_print_loop:
    pop rax                 ; dépile dans l'ordre (poids fort d'abord)
    add al, '0'             ; convertit en ASCII
    call put_char
    dec rcx
    jnz .pd_print_loop

    pop r13
    pop r12
    pop rbx
    leave
    ret

; ==============================================================================
; Section .data : Constantes et Chaînes Littérales
; ==============================================================================
section .data
    db_filename:        db "vault.db", 0

    msg_banner:         db 10, "========================================", 10 \
                           , "     SECURE VAULT - GESTIONNAIRE     ", 10 \
                           , "========================================", 10, 0

    msg_menu:           db 10, "--- MENU PRINCIPAL ---", 10 \
                           , "1. Ajouter un identifiant (F1)", 10 \
                           , "2. Afficher les identifiants dechiffres (F2)", 10 \
                           , "3. Quitter", 10 \
                           , "Votre choix [1-3] : ", 0

    msg_bad_choice:     db "[-] Choix invalide, veuillez reessayer.", 10, 0
    msg_quit:           db "[*] Fermeture de Secure Vault. Au revoir.", 10, 0

    msg_f0_prompt:      db "Entrez le mot de passe maitre de la base : ", 0
    msg_f0_ok:          db "[+] Base de donnees 'vault.db' initialisee avec succes.", 10, 0
    msg_f0_warn_empty:  db "[!] Attention : mot de passe vide utilise.", 10, 0
    msg_f0_err:         db "[-] Erreur critique lors de la creation/ouverture de vault.db !", 10, 0

    msg_f1_title:       db 10, "[F1] AJOUT D'UN NOUVEL IDENTIFIANT", 10, 0
    msg_f1_login:       db "  Login (max 31 car.)    : ", 0
    msg_f1_pass:        db "  Password (max 31 car.) : ", 0
    msg_f1_ok:          db "[+] Identifiant ajoute et base rechiffree avec succes.", 10, 0
    msg_f1_full:        db "[-] Erreur : Capacite maximale atteinte (64 comptes max).", 10, 0

    msg_f2_title:       db 10, "[F2] REPERTOIRE DES IDENTIFIANTS (DECHIFFRE) :", 10, 0
    msg_f2_empty:       db "[*] La base de donnees est actuellement vide (0 compte).", 10, 0
    msg_sep:            db "----------------------------------------", 10, 0
    msg_entry_num:      db "Compte #", 0
    msg_lbl_login:      db "  Login    : ", 0
    msg_lbl_pass:       db "  Password : ", 0
    msg_newline:        db 10, 0

    msg_err_read:       db "[-] Erreur d'I/O lors de la lecture du fichier vault.db !", 10, 0
    msg_err_write:      db "[-] Erreur d'I/O lors de l'ecriture du fichier vault.db !", 10, 0
    msg_err_corrupt:    db "[-] ALERTE : Fichier 'vault.db' corrompu (taille non multiple de 64) !", 10, 0

; ==============================================================================
; Section .bss : Variables Non Initialisées (Mémoire Allouée au Chargement)
; ==============================================================================
section .bss
    master_key:         resb KEY_LEN            ; 8 octets pour la clé dérivée F0
    input_buf:          resb 128                ; Tampon de saisie pour menus et prompts
    temp_cred:          resb credential_size    ; 64 octets pour la structure en cours de saisie
    db_buffer:          resb DB_MAX_SIZE        ; 4096 octets pour la base complète en mémoire
