#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/sha.h>
#include "/usr/local/include/secp256k1.h"
#include <mbedtls/ripemd160.h>

#define BATCH_SIZE 1000000 // Adjust to control the number of keys processed per batch
#define MAX_FILE_SIZE 500 * 1024 * 1024 // 500 MB in bytes

static const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

size_t base58_encode(const unsigned char* input, size_t length, char* output) {
    uint8_t buffer[32] = {0};
    size_t buffer_len = 0;
    size_t zeros = 0;
    size_t i, j;

    while (zeros < length && input[zeros] == 0) {
        zeros++;
    }

    for (i = zeros; i < length; i++) {
        uint16_t carry = input[i];
        for (j = 0; j < buffer_len; j++) {
            carry += (uint16_t)buffer[j] << 8;
            buffer[j] = carry % 58;
            carry /= 58;
        }
        while (carry > 0) {
            buffer[buffer_len++] = carry % 58;
            carry /= 58;
        }
    }

    size_t output_len = 0;
    for (i = 0; i < zeros; i++) {
        output[output_len++] = BASE58_ALPHABET[0];
    }
    for (i = buffer_len; i > 0; i--) {
        output[output_len++] = BASE58_ALPHABET[buffer[i - 1]];
    }
    output[output_len] = '\0';

    return output_len;
}

void generate_keys_and_addresses(uint64_t start_key, uint64_t num_keys, FILE *file) {
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    
    for (uint64_t i = 0; i < num_keys; i++) {
        unsigned char private_key[32];
        unsigned char public_key[65];
        unsigned char sha256_hash[32];
        unsigned char ripemd160_hash[20];
        unsigned char address_bytes[25];
        char address[40];
        size_t pubkey_len = 33;

        // Generate private key
        for (int j = 0; j < 32; j++) {
            private_key[j] = (start_key >> (8 * (31 - j))) & 0xFF;
        }

        // Generate the public key
        secp256k1_pubkey pubkey;
        if (secp256k1_ec_pubkey_create(ctx, &pubkey, private_key)) {
            secp256k1_ec_pubkey_serialize(ctx, public_key, &pubkey_len, &pubkey, SECP256K1_EC_COMPRESSED);
            
            // SHA256 hash of the public key
            SHA256(public_key, pubkey_len, sha256_hash);
            
            // RIPEMD160 of the SHA256 hash
            mbedtls_ripemd160_context ripemd160_ctx;
            mbedtls_ripemd160_init(&ripemd160_ctx);
            mbedtls_ripemd160_starts(&ripemd160_ctx);
            mbedtls_ripemd160_update(&ripemd160_ctx, sha256_hash, 32);
            mbedtls_ripemd160_finish(&ripemd160_ctx, ripemd160_hash);
            mbedtls_ripemd160_free(&ripemd160_ctx);

            // Prepare address bytes
            address_bytes[0] = 0x00;  // Version byte
            memcpy(address_bytes + 1, ripemd160_hash, 20);

            // Checksum (double SHA256)
            SHA256(address_bytes, 21, sha256_hash);
            SHA256(sha256_hash, 32, sha256_hash);
            memcpy(address_bytes + 21, sha256_hash, 4);

            // Base58 encode
            base58_encode(address_bytes, 25, address);

            // Export to CSV format
            fprintf(file, "\"0x%016llx\",\"%s\"\n", start_key + i, address);
        }
    }
    secp256k1_context_destroy(ctx);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <start_key_range> <end_key_range>\n", argv[0]);
        return 1;
    }

    uint64_t start_key = strtoull(argv[1], NULL, 16);
    uint64_t end_key = strtoull(argv[2], NULL, 16);
    uint64_t num_keys = end_key - start_key + 1;
    uint64_t total_keys_generated = 0;
    
    char filename[100];
    int file_counter = 0;

    while (total_keys_generated < num_keys) {
        snprintf(filename, sizeof(filename), "/app/output/keys_addresses_%d.csv", file_counter);
        FILE *file = fopen(filename, "w");
        if (!file) {
            printf("Error opening file for writing.\n");
            return 1;
        }

        fprintf(file, "private_key,address\n");

        size_t current_file_size = 0;
        while (current_file_size < MAX_FILE_SIZE && total_keys_generated < num_keys) {
            uint64_t keys_to_generate = (num_keys - total_keys_generated < BATCH_SIZE) ? num_keys - total_keys_generated : BATCH_SIZE;
            generate_keys_and_addresses(start_key + total_keys_generated, keys_to_generate, file);
            total_keys_generated += keys_to_generate;
            current_file_size = ftell(file);
        }

        fclose(file);
        file_counter++;
    }

    printf("Key generation completed. Total keys generated: %llu\n", total_keys_generated);
    return 0;
}