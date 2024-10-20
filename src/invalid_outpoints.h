#ifndef INVALID_OUTPOINTS_H
#define INVALID_OUTPOINTS_H

#include <string>

// Function to return a JSON string representing invalid outpoints
std::string LoadInvalidOutPoints() {
    // This is a placeholder JSON structure for invalid outpoints
    return "[\n"
           " {\n"
           "   \"txid\": \"0000000000000000000000000000000000000000000000000000000000000000\",\n"
           "   \"index\": 0\n"
           " }\n"
           "]";
}

#endif // INVALID_OUTPOINTS_H
