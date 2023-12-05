
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void printWithLineNumbers(const char* text) {
    // Initialize line number
    int lineNumber = 1;

    // Get the length of the text
    size_t textLength = strlen(text);

    // Buffer to hold formatted output
    char buffer[BUFSIZ];

    // Iterate through each character in the text
    for (size_t i = 0; i < textLength; ++i) {
        // Format line number and increment for each new line
        if (i == 0 || (i > 0 && text[i - 1] == '\n')) {
            int lineNumberLength = snprintf(buffer, sizeof(buffer), "%d: ", lineNumber);
            write(STDOUT_FILENO, buffer, lineNumberLength);
            ++lineNumber;
        }

        // Write the current character to the screen
        write(STDOUT_FILENO, &text[i], 1);
    }
}

int main() {
    // Example text
    const char* exampleText;

    // Print text with line numbers
    printWithLineNumbers(exampleText);

    return 0;
}

