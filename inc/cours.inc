; put_char : ecrit al sur stdout  (= l'ancien mov [_print_char], al du TD05)
put_char:
    push rdi
    push rsi
    push rdx
    push rcx                ; syscall ecrase rcx et r11 -> on les sauve
    push r11
    push rax
    mov rsi, rsp            ; pointe l'octet (al)
    mov rax, 1             ; write
    mov rdi, 1             ; stdout
    mov rdx, 1
    syscall
    pop rax
    pop r11
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    ret

; put_hex : ecrit al en 2 chiffres hexa  (= l'ancien mov [_print_hex], al)
put_hex:
    push rbx
    movzx rbx, al
    mov al, bl             ; nibble haut
    shr al, 4
    cmp al, 0xa
    jb put_hex_hi0
    add al, 0x57
    jmp put_hex_hi
put_hex_hi0:
    add al, 0x30
put_hex_hi:
    call put_char
    mov al, bl             ; nibble bas
    and al, 0xf
    cmp al, 0xa
    jb put_hex_lo0
    add al, 0x57
    jmp put_hex_lo
put_hex_lo0:
    add al, 0x30
put_hex_lo:
    call put_char
    pop rbx
    ret

; strlen(char* rdi) -> rax
strlen:
    xor rax, rax
strlen_loop:
    cmp byte [rdi], 0
    je strlen_done
    inc rdi
    inc rax
    jmp strlen_loop
strlen_done:
    ret

; print(char* rdi)
print:
    mov rsi, rdi
print_loop:
    mov al, [rsi]
    cmp al, 0
    je print_done
    call put_char
    inc rsi
    jmp print_loop
print_done:
    ret

; print_hex(char* rdi, long rsi) : rsi octets en hexa
print_hex:
    mov rcx, rsi            ; compteur
    mov rsi, rdi            ; pointeur
print_hex_loop:
    cmp rcx, 0
    je print_hex_done
    mov al, [rsi]
    call put_hex
    inc rsi
    dec rcx
    jmp print_hex_loop
print_hex_done:
    ret

; int_print_hex(long rdi) : un entier 64 bits en hexa (poids fort d'abord)
int_print_hex:
    mov rcx, 8
int_print_hex_loop:
    rol rdi, 8
    mov al, dil
    call put_hex
    dec rcx
    jnz int_print_hex_loop
    ret

; strcpy(char* rdi, char* rsi)   dst, src
strcpy:
strcpy_loop:
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    cmp al, 0
    jne strcpy_loop
    ret

; strncpy(char* rdi, char* rsi, long rdx)   dst, src, n
strncpy:
strncpy_loop:
    cmp rdx, 0
    je strncpy_done
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rdx
    cmp al, 0
    jne strncpy_loop
strncpy_pad:                ; src fini -> on complete avec des \0
    cmp rdx, 0
    je strncpy_done
    mov byte [rdi], 0
    inc rdi
    dec rdx
    jmp strncpy_pad
strncpy_done:
    ret

; strcmp(char* rdi, char* rsi) -> rax (0 si egales)
strcmp:
strcmp_loop:
    mov al, [rdi]
    mov bl, [rsi]
    cmp al, bl
    jne strcmp_diff
    cmp al, 0
    je strcmp_equal
    inc rdi
    inc rsi
    jmp strcmp_loop
strcmp_diff:
    movzx rax, al
    movzx rbx, bl
    sub rax, rbx
    ret
strcmp_equal:
    xor rax, rax
    ret

; readn_string(char* rdi, long rsi) -> rax = octets lus   (1 syscall, cap size)
readn_string:
    mov rdx, rsi            ; count = size
    mov rsi, rdi            ; buf = dst
    xor rdi, rdi            ; fd = 0 (stdin)
    xor rax, rax            ; read = 0
    syscall
    ret

; read_string(char* rdi, long rsi)   dst, size : lit jusqu'au \n, termine par \0
read_string:
    mov rcx, rsi            ; compteur restant
rs_loop:
    cmp rcx, 0
    je rs_done
    push rdi
    push rcx
    mov rsi, rdi            ; buffer = position courante
    xor rdi, rdi            ; stdin
    xor rax, rax            ; read
    mov rdx, 1              ; 1 octet
    syscall
    pop rcx
    pop rdi
    cmp rax, 0              ; EOF ?
    je rs_term
    cmp byte [rdi], 0x0a  ; \n ?
    je rs_term
    inc rdi
    dec rcx
    jmp rs_loop
rs_term:
    mov byte [rdi], 0     ; remplace \n (ou fin) par \0
rs_done:
    ret

; read_hex() -> rax = valeur (lit une saisie hexa dans un buffer interne)
read_hex:
    mov rdi, read_hex_buf   ; buffer interne
    mov rsi, 0x20           ; taille max
    call read_string
    mov rsi, read_hex_buf   ; reparcours pour parser
    xor rax, rax
rh_loop:
    movzx rcx, byte [rsi]
    cmp cl, 0x30            ; < '0' -> fin
    jb rh_done
    cmp cl, 0x39
    jbe rh_digit
    or cl, 0x20             ; tolower
    cmp cl, 0x61
    jb rh_done
    cmp cl, 0x66
    ja rh_done
    sub cl, 0x57            ; 'a' -> 10
    jmp rh_acc
rh_digit:
    sub cl, 0x30
rh_acc:
    shl rax, 4
    add rax, rcx
    inc rsi
    jmp rh_loop
rh_done:
    ret

; read_char(char* rdi) : lit 1 octet dans dst
read_char:
    mov rsi, rdi            ; buf = dst
    xor rdi, rdi            ; fd = 0 (stdin)
    xor rax, rax            ; read = 0
    mov rdx, 1              ; 1 octet
    syscall
    ret

; open_file(char* rdi) -> rax = fd   (O_RDWR|O_CREAT, 0644)
open_file:
    mov rsi, 0x42           ; O_RDWR | O_CREAT
    mov rdx, 0x1a4          ; 0644
    mov rax, 2             ; open
    syscall
    ret

; write_file(int rdi=fd, char* rsi=buf, long rdx=size) -> rax
write_file:
    mov rax, 1             ; write
    syscall
    ret

; read_file(int rdi=fd, char* rsi=buf, long rdx=size) -> rax
read_file:
    xor rax, rax            ; read
    syscall
    ret

; close_file(int rdi=fd)
close_file:
    mov rax, 3             ; close
    syscall
    ret

; print_string(char* rdi, long rsi) : ecrit rsi octets sur stdout (1 write)
print_string:
    mov rdx, rsi            ; count = size
    mov rsi, rdi            ; buf
    mov rdi, 1              ; stdout
    mov rax, 1              ; write
    syscall
    ret

; is_palindrome(char* rdi) -> rax (0 si palindrome) : recopie a l'envers et compare
is_palindrome:
    enter 16, 0
    mov [rbp - 8], rdi       ; str
    call strlen              ; rax = longueur
    mov [rbp - 16], rax      ; len
    mov rsi, [rbp - 8]       ; src = str
    mov rdi, is_palindrome_buf
    mov rcx, [rbp - 16]      ; rcx = len
ip_reverse_loop:
    cmp rcx, 0
    je ip_reverse_done
    mov al, [rsi + rcx - 1]  ; dernier caractere en premier
    mov [rdi], al
    inc rdi
    dec rcx
    jmp ip_reverse_loop
ip_reverse_done:
    mov byte [rdi], 0        ; terminateur
    mov rdi, [rbp - 8]       ; str
    mov rsi, is_palindrome_buf
    call strcmp              ; 0 si egales = palindrome
    leave
    ret

; strnlen(char* rdi, long rsi) -> rax (longueur plafonnee a rsi)
strnlen:
    xor rax, rax
strnlen_loop:
    cmp rax, rsi
    jae strnlen_done
    cmp byte [rdi + rax], 0
    je strnlen_done
    inc rax
    jmp strnlen_loop
strnlen_done:
    ret

; strncmp(char* rdi, char* rsi, long rdx) -> rax (0 si egales sur n octets)
strncmp:
    xor rcx, rcx
strncmp_loop:
    cmp rcx, rdx
    je strncmp_equal
    mov al, [rdi + rcx]
    mov bl, [rsi + rcx]
    cmp al, bl
    jne strncmp_diff
    cmp al, 0
    je strncmp_equal
    inc rcx
    jmp strncmp_loop
strncmp_diff:
    movzx rax, al
    movzx rbx, bl
    sub rax, rbx
    ret
strncmp_equal:
    xor rax, rax
    ret

; memcpy(char* rdi, char* rsi, long rdx)   dst, src, n
memcpy:
    xor rcx, rcx
memcpy_loop:
    cmp rcx, rdx
    je memcpy_done
    mov al, [rsi + rcx]
    mov [rdi + rcx], al
    inc rcx
    jmp memcpy_loop
memcpy_done:
    ret

; memcmp(char* rdi, char* rsi, long rdx) -> rax (0 si identiques sur exactement n octets)
memcmp:
    xor rcx, rcx
memcmp_loop:
    cmp rcx, rdx
    je memcmp_equal
    mov al, [rdi + rcx]
    mov bl, [rsi + rcx]
    cmp al, bl
    jne memcmp_diff
    inc rcx
    jmp memcmp_loop
memcmp_diff:
    movzx rax, al
    movzx rbx, bl
    sub rax, rbx
    ret
memcmp_equal:
    xor rax, rax
    ret

; memset(char* rdi, byte sil, long rdx)   dst, val, n
memset:
    xor rcx, rcx
memset_loop:
    cmp rcx, rdx
    je memset_done
    mov [rdi + rcx], sil
    inc rcx
    jmp memset_loop
memset_done:
    ret

; xor_stream(char* rdi=dst, char* rsi=src, long rdx=len, char* rcx=key)
; chiffre ou dechiffre len octets avec la cle en rcx (XOR cyclique)
xor_stream:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi            ; dst
    mov r13, rsi            ; src
    mov r14, rdx            ; len
    mov r15, rcx            ; key

    mov rdi, r15
    call strlen
    test rax, rax
    jz xor_stream_done
    mov r8, rax             ; r8 = key_len

    xor r9, r9              ; r9 = index (0 .. len-1)
xor_stream_loop:
    cmp r9, r14
    jae xor_stream_done

    mov rax, r9
    xor rdx, rdx
    div r8                  ; rdx = r9 % key_len

    movzx ebx, byte [r15 + rdx] ; octet de la cle
    movzx eax, byte [r13 + r9]  ; octet source
    xor al, bl

    mov [r12 + r9], al      ; destination
    inc r9
    jmp xor_stream_loop

xor_stream_done:
    mov byte [r12 + r14], 0 ; terminaison nulle
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; file_create_write(char* rdi=path, char* rsi=buf, long rdx=len) -> rax (octets ecrits)
file_create_write:
    push rbx
    push r12
    push r13

    mov r12, rsi            ; buf
    mov r13, rdx            ; len

    mov rax, 87             ; sys_unlink preventif
    syscall

    call open_file
    cmp rax, 0
    jl fcw_err
    mov rbx, rax            ; rbx = fd

    mov rdi, rbx
    mov rsi, r12
    mov rdx, r13
    call write_file
    mov r12, rax            ; r12 = octets ecrits

    mov rdi, rbx
    call close_file

    mov rax, r12
    jmp fcw_done
fcw_err:
fcw_done:
    pop r13
    pop r12
    pop rbx
    ret

; file_read_all(char* rdi=path, char* rsi=buf, long rdx=max_len) -> rax (octets lus)
file_read_all:
    push rbx
    push r12
    push r13

    mov r12, rsi            ; buf
    mov r13, rdx            ; max_len

    call open_file
    cmp rax, 0
    jl fra_err
    mov rbx, rax            ; fd

    mov rdi, rbx
    mov rsi, r12
    mov rdx, r13
    call read_file
    mov r12, rax            ; octets lus

    mov rdi, rbx
    call close_file

    mov rax, r12
    jmp fra_done
fra_err:
fra_done:
    pop r13
    pop r12
    pop rbx
    ret

; safe_read_string(char* rdi=dst, long rsi=size) -> rax = longueur reelle
; Lit jusqu'au \n ou size-1 octets, garantit TOUJOURS le \0, et renvoie la taille dans rax
safe_read_string:
    push rbx
    push r12
    push r13

    mov r12, rdi            ; tampon destination
    mov r13, rsi            ; taille maximale du tampon

    ; Si size <= 1, on ne peut stocker qu'un \0 ou rien
    cmp r13, 1
    jbe .srs_empty

    ; 1. Lecture de size-1 octets max via readn_string (1 seul syscall direct)
    mov rdi, r12
    lea rsi, [r13 - 1]      ; laisse au moins 1 octet pour le \0
    call readn_string       ; rax = nombre d'octets reellement lus
    test rax, rax
    jle .srs_empty

    ; 2. Verification avec strnlen (s'arrete au premier \0 ou au bout de ce qui a ete lu)
    mov rdi, r12
    mov rsi, rax
    call strnlen
    mov rbx, rax

    ; 3. Si le dernier caractere est un \n (0x0A), on le supprime (chaine propre)
    test rbx, rbx
    jz .srs_terminate
    cmp byte [r12 + rbx - 1], 0x0a
    jne .srs_terminate
    dec rbx

.srs_terminate:
    mov byte [r12 + rbx], 0 ; pose inconditionnellement le \0
    mov rax, rbx            ; retourne la taille reelle dans rax
    jmp .srs_done

.srs_empty:
    test r13, r13
    jz .srs_done
    mov byte [r12], 0       ; chaine vide terminee
    xor rax, rax

.srs_done:
    pop r13
    pop r12
    pop rbx
    ret

; ==============================================================================
; PRIMITIVES SPÉCIFIQUES SECURE VAULT (TD08)
; ==============================================================================

; xor_ecb8(char* rdi=buf, long rsi=len, char* rdx=key8)
; Chiffre ou déchiffre in-place len octets par bloc de 8 octets (ECB)
xor_ecb8:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov r12, rdi            ; r12 = tampon
    mov r13, rsi            ; r13 = longueur en octets
    mov r14, rdx            ; r14 = adresse de la clé (8 octets)

    xor rcx, rcx            ; index = 0
.xe8_loop:
    cmp rcx, r13
    jae .xe8_done

    ; Modulo 8 optimisé sans div : index & 7
    mov rax, rcx
    and rax, 7

    movzx ebx, byte [r14 + rax] ; Octet de la clé (0 à 7)
    xor byte [r12 + rcx], bl    ; XOR in-place

    inc rcx
    jmp .xe8_loop

.xe8_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret

; print_cstr_n(char* rdi=buf, long rsi=max_len)
; Affiche au plus max_len caractères, s'arrête au premier \0
print_cstr_n:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rdi            ; r12 = buf
    mov r13, rsi            ; r13 = max_len

    xor rcx, rcx
.pcn_count:
    cmp rcx, r13
    jae .pcn_print
    cmp byte [r12 + rcx], 0
    je .pcn_print
    inc rcx
    jmp .pcn_count

.pcn_print:
    test rcx, rcx
    jz .pcn_done

    mov rdi, r12
    mov rsi, rcx
    call print_string

.pcn_done:
    pop r13
    pop r12
    pop rbx
    leave
    ret

; read_line_bounded(char* rdi=dst, long rsi=max_len) -> rax = longueur réelle
; Lit depuis stdin jusqu'au \n ou EOF, stocke max_len - 1 caractères + \0.
; Purge les caractères excédentaires de la ligne courante.
read_line_bounded:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov r12, rdi            ; dst
    mov r13, rsi            ; max_len
    xor r14, r14            ; count = 0

    cmp r13, 1
    jbe .rlb_empty

    dec r13                 ; réserve 1 octet pour le \0

.rlb_loop:
    sub rsp, 16             ; espace pile aligné 16 octets
    xor rdi, rdi            ; fd = 0 (stdin)
    mov rsi, rsp            ; buffer = pile
    mov rdx, 1              ; 1 octet
    xor rax, rax            ; sys_read = 0
    syscall

    test rax, rax
    jle .rlb_eof

    movzx eax, byte [rsp]
    add rsp, 16

    cmp al, 0x0a            ; \n détecté ?
    je .rlb_finished

    cmp r14, r13
    jae .rlb_loop           ; si plein, ignore le caractère (purge stdin)

    mov [r12 + r14], al     ; stocke le caractère valide
    inc r14
    jmp .rlb_loop

.rlb_eof:
    add rsp, 16
.rlb_finished:
    mov byte [r12 + r14], 0 ; pose inconditionnellement le \0
    mov rax, r14            ; retourne la taille
    jmp .rlb_exit

.rlb_empty:
    test r13, r13
    jz .rlb_zero
    mov byte [r12], 0
.rlb_zero:
    xor rax, rax

.rlb_exit:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret

; buffers internes (on repasse en .text ensuite pour le code suivant)
section .bss
    read_hex_buf: resb 0x20
    is_palindrome_buf: resb 256
section .text

