#include "cache.hh"
#include <assert.h>
#include <unordered_map>
#include <algorithm>
#include <functional>

#define BUCKET_COUNT 10

// dev  
// #include <iostream>

class Cache::Impl {
    private:
        // private data structure
        Evictor* evictor;
        float max_load_factor;
        size_type maxmem;
        hash_func hasher;
        size_type cur_size = 0;
        std::unordered_map<key_type, val_type, hash_func> table;

    public:
        // Add a <key, value> pair to the cache.
        // If key already exists, it will overwrite the old value.
        // Both the key and the value are to be deep-copied (not just pointer copied).
        // If maxmem capacity is exceeded, enough values will be removed
        // from the cache to accomodate the new value. If unable, the new value
        // isn't inserted to the cache.
        void set(key_type key, val_type val, size_type size) {
            if (space_used() + size <= maxmem || table.count(key) != 0) {
                cur_size += size;
                table[key] = val;
                if (evictor != nullptr)
                    evictor->touch_key(key);
            } else if (evictor != nullptr) {
                key_type to_evict = evictor->evict();
                this -> del(to_evict);
                cur_size += size;
                table[key] = val;
                evictor->touch_key(key);
            }
            // don't add into hash table
        }

        // Retrieve a pointer to the value associated with key in the cache,
        // or nullptr if not found.
        // Sets the actual size of the returned value (in bytes) in val_size.
        val_type get(key_type key, size_type& val_size) const {
            // table.at() returns a reference to the value
            if (table.count(key) != 0) {

                // testing resizing with max_load_factor stuff
                /*
                int x = this -> get_bucket_count();
                std::cout << x << std::endl;
                */

                if (evictor != nullptr)
                    evictor->touch_key(key);
                val_size = sizeof(*table.at(key));

                return table.at(key);
            } else {
                return nullptr;
            }
        }

        // Delete an object from the cache, if it's still there
        bool del(key_type key) {
            size_type val_size;
            this -> get(key, val_size);
            cur_size -= val_size;
            return table.erase(key);
        }

        // Compute the total amount of memory used up by all cache values (not keys)
        size_type space_used() const {
            return cur_size;
        }

        // Delete all data from the cache
        void reset() {
            cur_size = 0;
            table.clear();
        }

        // This function is used to test the resizing using max_load_factor
        
        /*
        int get_bucket_count() const{ 
        return table.bucket_count();
        }
        */

        Impl(size_type maxmem,
              float max_load_factor,
              Evictor* evictor,
              hash_func hasher) : maxmem(maxmem),
                    max_load_factor(max_load_factor),
                    evictor(evictor),
                    table(std::unordered_map<key_type, val_type, hash_func> (BUCKET_COUNT, hasher)) {
                        table.max_load_factor(max_load_factor);
                    }
};

// cache impl stuff

// constructor for the cache

Cache::Cache(size_type maxmem,
            float max_load_factor,
            Evictor* evictor,
            hash_func hasher)
    : pImpl_(new Impl(maxmem, max_load_factor, evictor, hasher)) {}

Cache::~Cache() = default;

void Cache::set(key_type key, val_type val, size_type size) {
    Cache::pImpl_->set(key, val, size);
}

// Retrieve a pointer to the value associated with key in the cache,
// or nullptr if not found.
// Sets the actual size of the returned value (in bytes) in val_size.
Cache::val_type Cache::get(key_type key, Cache::size_type& val_size) const {
    return Cache::pImpl_->get(key, val_size);
}

// Delete an object from the cache, if it's still there
bool Cache::del(key_type key){
    return Cache::pImpl_->del(key);
}

// Compute the total amount of memory used up by all cache values (not keys)
Cache::size_type Cache::space_used() const {
    return Cache::pImpl_->space_used();
}
// Delete all data from the cache
void Cache::reset(){
    Cache::pImpl_->reset();
}
