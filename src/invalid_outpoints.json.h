#ifndef INVALID_OUTPOINTS_JSON_H
#define INVALID_OUTPOINTS_JSON_H

std::string GetInvalidOutpoints() {
    return "[\n"
           " {\n"
           "   \"txid\": \"\",\n"
           "   \"index\": 0\n"
           " }\n"
           "]";
}

#endif // INVALID_OUTPOINTS_JSON_H
