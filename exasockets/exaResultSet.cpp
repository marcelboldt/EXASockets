//
// Created by mbo on 02.11.2016.
//

#include "exaResultSet.h"


}

exaTblColumn::exaTblColumn(char *name, int datatype, int precision, int scale) {

    this->name = *name;
    switch (datatype) {
        case BOOLEAN :
            this->type = BOOLEAN;
            this->data = new exaColData<bool>();
            break;
        case CHAR :
            this->type = CHAR;
            this->data = new exaColData<char>();
            break;
        default:
            throw "unknown datatype";
    }
}

}

c = new exaTblColumn("col1", BOOLEAN);
c.

appendData(char *data, size_t start = 1;

size_t stop = 0
);
c[10]; // returns row 10


