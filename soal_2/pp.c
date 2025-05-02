#include <stdio.h>
#include <pthread.h>

// Struktur shared memory
typedef struct {
    pthread_mutex_t lock;  // Mutex untuk sinkronisasi
    int some_data;         // Contoh data yang akan diakses bersama
} SharedData;

void *thread_function(void *arg) {
    SharedData *shared_data = (SharedData *)arg;
    
    // Mengunci mutex sebelum mengakses shared data
    pthread_mutex_lock(&shared_data->lock);

    // Akses shared data di sini
    shared_data->some_data = 42;

    // Membuka kunci mutex setelah selesai
    pthread_mutex_unlock(&shared_data->lock);

    return NULL;
}

int main() {
    SharedData shared_data;
    
    // Inisialisasi mutex
    if (pthread_mutex_init(&shared_data.lock, NULL) != 0) {
        printf("Mutex init failed\n");
        return 1;
    }

    // Membuat thread
    pthread_t thread;
    pthread_create(&thread, NULL, thread_function, (void *)&shared_data);

    // Menunggu thread selesai
    pthread_join(thread, NULL);

    // Menghancurkan mutex setelah selesai
    pthread_mutex_destroy(&shared_data.lock);

    return 0;
}
