//Includes
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>




//Defines
#define CTRL_KEY(k) ((k) & 0x1f)

#define ZILO_VERSION "0.0.1"

//Data

struct editorConfig {
  int cx, cy;
  int screenrows;
  int screencols;
  struct termios orig_termios;
};


struct editorConfig E;
//Terminal
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
  exit(1);
}

void disableRawMode() {
   if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
    if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return 'k';
        case 'B': return 'j';
        case 'C': return 'l';
        case 'D': return 'h';
      }
    }
    return '\x1b';
  } else {
    return c;
  }

  
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/
struct abuf {
  char *b;
  int len;
};
#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}



/*** output ***/

/*int editorDrawLineNumbers() {
    int x;
    int lineNum;
    char tmp[10] = {0x0};
    for(x = 1; x < E.screenrows - 1; x++) {
        lineNum = x;
        sprintf(tmp, "%d", lineNum);
        write(STDOUT_FILENO, tmp, sizeof(tmp));
        write(STDOUT_FILENO, "\r\n", 3);
    }

}
*/


int editorDrawRows(struct abuf *ab) {
     int x;
    int lineNum;
    char tmp[10] = {0x0};
    for(x = 1; x < E.screenrows / 1; x++) {
        if (x == E.screenrows - 1) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                "Zilo Editor -- version %s", ZILO_VERSION);
            if (welcomelen > E.screencols) welcomelen = E.screencols;
            int padding = (E.screencols - welcomelen) / 2;
            if (padding) {
            abAppend(ab, "~", 1);
            padding--;
      }
      while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, welcome, welcomelen);
        } else {

        lineNum = x;
        sprintf(tmp, "%d", lineNum);
        abAppend(ab, tmp, sizeof(tmp));
        }
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 3);
    }
    
 }
void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);

  //abAppend(&ab, "\x1b[2J", 4);
  abAppend(&ab, "\x1b[H", 3);
  
  editorDrawRows(&ab);
  
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf));

  //abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** input ***/

void editorMoveCursor(char key) {
    switch(key) {
     case 'h':
        E.cx--;
        break;
     case 'l':
        E.cx++;
        break;
     case 'j':
        E.cy++;
        break;
     case 'k':
        E.cy--;
        break;
    }
}



void editorProcessKeypress() {
  char c = editorReadKey();
   switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case 'k':
    case 'j':
    case 'h':
    case 'l':
      editorMoveCursor(c);
      break;
  }
}






// Initialize

void initEditor() {
    E.cx = 0;
    E.cy = 0;


  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}


int main() {
 enableRawMode();
 initEditor();
 char c;
 
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
 
}
