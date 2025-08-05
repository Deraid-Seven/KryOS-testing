/*
 * kernel.c - version 0.0.4
 */

#define WHITE_TXT 0x07
#define KEY_ENTER 0x1C
#define KEY_BACKSPACE 0x0E
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Объявляем функции
void k_clear_screen(void);
void k_printf(const char *message, unsigned int line);
void k_handle_command(const char *command, unsigned int line);
int strcmp(const char *s1, const char *s2);
unsigned char inb(unsigned short port);
void keyboard_wait(void);
unsigned char keyboard_read(void);
void scroll_screen(void);
void print_prompt(unsigned int line);

// Таблица преобразования скан-кодов
const char kbd_set1[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Глобальная переменная для текущей позиции вывода
unsigned int current_display_line = 0;

/* Точка входа ядра */
void k_main() {
    k_clear_screen();
    k_printf("Kernel initialized", 0);
    
    // Простая задержка (очень приблизительная)
    for (volatile long i = 0; i < 20000000; i++) {}
    
    k_clear_screen();
    current_display_line = 0;
    
    char command_buf[100];
    int buf_index = 0;
    
    print_prompt(current_display_line);
    
    while(1) {
        unsigned char scancode = keyboard_read();
        
        // Игнорируем отпускание клавиш
        if(scancode & 0x80) continue;
        
        char c = kbd_set1[scancode];
        
        if(c == '\n') { // Enter
            command_buf[buf_index] = '\0';
            
            // Обработка команды
            if(buf_index > 0) {
                k_handle_command(command_buf, current_display_line);
                current_display_line++;
                if(current_display_line >= 24) scroll_screen();
            }
            
            // Подготовка к новому вводу
            buf_index = 0;
            command_buf[0] = '\0';
            
            // Печать нового приглашения
            current_display_line++;
            if(current_display_line >= 24) scroll_screen();
            print_prompt(current_display_line);
        }
        else if(c == '\b') { // Backspace
            if(buf_index > 0) {
                buf_index--;
                
                // Удаляем символ с экрана
                char *vidmem = (char*)0xB8000 + (current_display_line * 80 + buf_index + 2) * 2;
                *vidmem = ' ';
            }
        }
        else if(c && buf_index < sizeof(command_buf) - 1) {
            command_buf[buf_index] = c;
            buf_index++;
            command_buf[buf_index] = '\0';
            
            // Выводим символ на экран
            char *vidmem = (char*)0xB8000 + (current_display_line * 80 + buf_index + 1) * 2;
            *vidmem = c;
        }
    }
}

/* Безопасное чтение из порта */
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Ожидание доступности данных */
void keyboard_wait(void) {
    while(1) {
        if(inb(KEYBOARD_STATUS_PORT) & 1) {
            break;
        }
    }
}

/* Чтение скан-кода с клавиатуры */
unsigned char keyboard_read(void) {
    keyboard_wait();
    return inb(KEYBOARD_DATA_PORT);
}

/* Вывод приглашения */
void print_prompt(unsigned int line) {
    char *vidmem = (char *)0xB8000 + (line * 80 * 2);
    *vidmem++ = '>';
    *vidmem++ = WHITE_TXT;
    *vidmem++ = ' ';
    *vidmem = WHITE_TXT;
}

/* Обработчик команд */
void k_handle_command(const char *command, unsigned int line) {
    if(strcmp(command, "help") == 0) {
        k_printf("nobody gonna help you bro", line + 1);
    } else {
        k_printf("Unknown command", line + 1);
    }
}

/* Прокрутка экрана */
void scroll_screen(void) {
    char *vidmem = (char *)0xB8000;
    
    // Копируем строки на одну вверх
    for(unsigned int i = 0; i < 24 * 80 * 2; i++) {
        vidmem[i] = vidmem[i + 80 * 2];
    }
    
    // Очищаем последнюю строку
    for(unsigned int i = 24 * 80 * 2; i < 25 * 80 * 2; i += 2) {
        vidmem[i] = ' ';
        vidmem[i+1] = WHITE_TXT;
    }
    
    current_display_line = 23;
}

/* Сравнение строк */
int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Очистка экрана */
void k_clear_screen(void) {
    char *vidmem = (char *)0xB8000;
    for(unsigned int i = 0; i < 80 * 25 * 2; i += 2) {
        vidmem[i] = ' ';
        vidmem[i+1] = WHITE_TXT;
    }
    current_display_line = 0;
}

/* Вывод строки */
void k_printf(const char *message, unsigned int line) {
    // Корректировка если вышли за пределы экрана
    if(line >= 25) {
        scroll_screen();
        line = 24;
    }
    
    char *vidmem = (char *)0xB8000 + (line * 80 * 2);
    
    while(*message) {
        if(*message == '\n') {
            line++;
            if(line >= 25) {
                scroll_screen();
                line = 24;
            }
            vidmem = (char *)0xB8000 + (line * 80 * 2);
            message++;
        } else {
            *vidmem++ = *message++;
            *vidmem++ = WHITE_TXT;
        }
    }
}