#include "fifo_evictor.hh"


Fifo::Fifo(){}

void Fifo::touch_key(const key_type& key) {
        acess.push(key);// ?

}

const key_type Fifo::evict() {
        key_type to_remove = acess.front();
        acess.pop();
        return to_remove;
}
