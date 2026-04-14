#include <iostream>

#ifdef _WIN32
#include <conio.h>
char getch_cross() { return _getch(); }
#else
#include <termios.h>
#include <unistd.h>
char getch_cross() {
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return c;
}
#endif

int main() {
    char c;
    while (true) {
        c = getch_cross();
        if (c == 27) break;
        std::cout << c << std::flush;
    }
}