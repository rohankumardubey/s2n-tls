/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <string.h>

#include <openssl/crypto.h>

#include "error/s2n_errno.h"

#include "crypto/s2n_cipher.h"
#include "crypto/s2n_openssl.h"

#include "tls/s2n_auth_selection.h"
#include "tls/s2n_kex.h"
#include "tls/s2n_security_policies.h"
#include "tls/s2n_tls.h"
#include "tls/s2n_tls13.h"
#include "utils/s2n_safety.h"
#include "tls/s2n_psk.h"
#include "pq-crypto/s2n_pq.h"

/*************************
 * S2n Record Algorithms *
 *************************/
const struct s2n_record_algorithm s2n_record_alg_null = {
    .cipher = &s2n_null_cipher,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_rc4_md5 = {
    .cipher = &s2n_rc4,
    .hmac_alg = S2N_HMAC_MD5,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_rc4_sslv3_md5 = {
    .cipher = &s2n_rc4,
    .hmac_alg = S2N_HMAC_SSLv3_MD5,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_rc4_sha = {
    .cipher = &s2n_rc4,
    .hmac_alg = S2N_HMAC_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_rc4_sslv3_sha = {
    .cipher = &s2n_rc4,
    .hmac_alg = S2N_HMAC_SSLv3_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_3des_sha = {
    .cipher = &s2n_3des,
    .hmac_alg = S2N_HMAC_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_3des_sslv3_sha = {
    .cipher = &s2n_3des,
    .hmac_alg = S2N_HMAC_SSLv3_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_sha = {
    .cipher = &s2n_aes128,
    .hmac_alg = S2N_HMAC_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_sslv3_sha = {
    .cipher = &s2n_aes128,
    .hmac_alg = S2N_HMAC_SSLv3_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_sha_composite = {
    .cipher = &s2n_aes128_sha,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_sha256 = {
    .cipher = &s2n_aes128,
    .hmac_alg = S2N_HMAC_SHA256,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_sha256_composite = {
    .cipher = &s2n_aes128_sha256,
    .hmac_alg = S2N_HMAC_NONE,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sha = {
    .cipher = &s2n_aes256,
    .hmac_alg = S2N_HMAC_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sslv3_sha = {
    .cipher = &s2n_aes256,
    .hmac_alg = S2N_HMAC_SSLv3_SHA1,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sha_composite = {
    .cipher = &s2n_aes256_sha,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sha256 = {
    .cipher = &s2n_aes256,
    .hmac_alg = S2N_HMAC_SHA256,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sha256_composite = {
    .cipher = &s2n_aes256_sha256,
    .hmac_alg = S2N_HMAC_NONE,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_sha384 = {
    .cipher = &s2n_aes256,
    .hmac_alg = S2N_HMAC_SHA384,
    .flags = 0,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes128_gcm = {
    .cipher = &s2n_aes128_gcm,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = S2N_TLS12_AES_GCM_AEAD_NONCE,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_aes256_gcm = {
    .cipher = &s2n_aes256_gcm,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = S2N_TLS12_AES_GCM_AEAD_NONCE,
    .encryption_limit = UINT64_MAX,
};

const struct s2n_record_algorithm s2n_record_alg_chacha20_poly1305 = {
    .cipher = &s2n_chacha20_poly1305,
    .hmac_alg = S2N_HMAC_NONE,
    /* Per RFC 7905, ChaCha20-Poly1305 will use a nonce construction expected to be used in TLS1.3.
     * Give it a distinct 1.2 nonce value in case this changes.
     */
    .flags = S2N_TLS12_CHACHA_POLY_AEAD_NONCE,
    .encryption_limit = UINT64_MAX,
};

/* TLS 1.3 Record Algorithms */
const struct s2n_record_algorithm s2n_tls13_record_alg_aes128_gcm = {
    .cipher = &s2n_tls13_aes128_gcm,
    .hmac_alg = S2N_HMAC_NONE, /* previously used in 1.2 prf, we do not need this */
    .flags = S2N_TLS13_RECORD_AEAD_NONCE,
    .encryption_limit = S2N_TLS13_AES_GCM_MAXIMUM_RECORD_NUMBER,
};

const struct s2n_record_algorithm s2n_tls13_record_alg_aes256_gcm = {
    .cipher = &s2n_tls13_aes256_gcm,
    .hmac_alg = S2N_HMAC_NONE,
    .flags = S2N_TLS13_RECORD_AEAD_NONCE,
    .encryption_limit = S2N_TLS13_AES_GCM_MAXIMUM_RECORD_NUMBER,
};

const struct s2n_record_algorithm s2n_tls13_record_alg_chacha20_poly1305 = {
    .cipher = &s2n_chacha20_poly1305,
    .hmac_alg = S2N_HMAC_NONE,
    /* this mirrors s2n_record_alg_chacha20_poly1305 with the exception of TLS 1.3 nonce flag */
    .flags = S2N_TLS13_RECORD_AEAD_NONCE,
    .encryption_limit = UINT64_MAX,
};

/*********************
 * S2n Cipher Suites *
 *********************/

/* This is the initial cipher suite, but is never negotiated */
struct s2n_cipher_suite s2n_null_cipher_suite = {
    .available = 1,
    .name = "TLS_NULL_WITH_NULL_NULL",
    .iana_value = { TLS_NULL_WITH_NULL_NULL },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = &s2n_record_alg_null,
};

struct s2n_cipher_suite s2n_equal_preference_group_start = {
    .available = 0,
    .name = "EQUAL_PREFERENCE_GROUP_START",
    .iana_value = { TLS_NULL_WITH_NULL_NULL },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = &s2n_record_alg_null,
};

struct s2n_cipher_suite s2n_equal_preference_group_end = {
    .available = 0,
    .name = "EQUAL_PREFERENCE_GROUP_END",
    .iana_value = { TLS_NULL_WITH_NULL_NULL },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = &s2n_record_alg_null,
};

struct s2n_cipher_suite s2n_rsa_with_rc4_128_md5 = /* 0x00,0x04 */ {
    .available = 0,
    .name = "RC4-MD5",
    .iana_value = { TLS_RSA_WITH_RC4_128_MD5 },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_rc4_md5 },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_rc4_sslv3_md5,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_rsa_with_rc4_128_sha = /* 0x00,0x05 */ {
    .available = 0,
    .name = "RC4-SHA",
    .iana_value = { TLS_RSA_WITH_RC4_128_SHA },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_rc4_sha },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_rc4_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_rsa_with_3des_ede_cbc_sha = /* 0x00,0x0A */ {
    .available = 0,
    .name = "DES-CBC3-SHA",
    .iana_value = { TLS_RSA_WITH_3DES_EDE_CBC_SHA },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_3des_sha },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_3des_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_3des_ede_cbc_sha = /* 0x00,0x16 */ {
    .available = 0,
    .name = "DHE-RSA-DES-CBC3-SHA",
    .iana_value = { TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_3des_sha },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_3des_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_rsa_with_aes_128_cbc_sha = /* 0x00,0x2F */ {
    .available = 0,
    .name = "AES128-SHA",
    .iana_value = { TLS_RSA_WITH_AES_128_CBC_SHA },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha_composite, &s2n_record_alg_aes128_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes128_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_128_cbc_sha = /* 0x00,0x33 */ {
    .available = 0,
    .name = "DHE-RSA-AES128-SHA",
    .iana_value = { TLS_DHE_RSA_WITH_AES_128_CBC_SHA },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha_composite, &s2n_record_alg_aes128_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes128_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_rsa_with_aes_256_cbc_sha = /* 0x00,0x35 */ {
    .available = 0,
    .name = "AES256-SHA",
    .iana_value = { TLS_RSA_WITH_AES_256_CBC_SHA },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha_composite, &s2n_record_alg_aes256_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes256_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_256_cbc_sha = /* 0x00,0x39 */ {
    .available = 0,
    .name = "DHE-RSA-AES256-SHA",
    .iana_value = { TLS_DHE_RSA_WITH_AES_256_CBC_SHA },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha_composite, &s2n_record_alg_aes256_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes256_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_rsa_with_aes_128_cbc_sha256 = /* 0x00,0x3C */ {
    .available = 0,
    .name = "AES128-SHA256",
    .iana_value = { TLS_RSA_WITH_AES_128_CBC_SHA256 },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha256_composite, &s2n_record_alg_aes128_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_rsa_with_aes_256_cbc_sha256 = /* 0x00,0x3D */ {
    .available = 0,
    .name = "AES256-SHA256",
    .iana_value = { TLS_RSA_WITH_AES_256_CBC_SHA256 },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha256_composite, &s2n_record_alg_aes256_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_128_cbc_sha256 = /* 0x00,0x67 */ {
    .available = 0,
    .name = "DHE-RSA-AES128-SHA256",
    .iana_value = { TLS_DHE_RSA_WITH_AES_128_CBC_SHA256 },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha256_composite, &s2n_record_alg_aes128_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_256_cbc_sha256 = /* 0x00,0x6B */ {
    .available = 0,
    .name = "DHE-RSA-AES256-SHA256",
    .iana_value = { TLS_DHE_RSA_WITH_AES_256_CBC_SHA256 },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha256_composite, &s2n_record_alg_aes256_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_rsa_with_aes_128_gcm_sha256 = /* 0x00,0x9C */ {
    .available = 0,
    .name = "AES128-GCM-SHA256",
    .iana_value = { TLS_RSA_WITH_AES_128_GCM_SHA256 },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_rsa_with_aes_256_gcm_sha384 = /* 0x00,0x9D */ {
    .available = 0,
    .name = "AES256-GCM-SHA384",
    .iana_value = { TLS_RSA_WITH_AES_256_GCM_SHA384 },
    .key_exchange_alg = &s2n_rsa,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_128_gcm_sha256 = /* 0x00,0x9E */ {
    .available = 0,
    .name = "DHE-RSA-AES128-GCM-SHA256",
    .iana_value = { TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_aes_256_gcm_sha384 = /* 0x00,0x9F */ {
    .available = 0,
    .name = "DHE-RSA-AES256-GCM-SHA384",
    .iana_value = { TLS_DHE_RSA_WITH_AES_256_GCM_SHA384 },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_128_cbc_sha = /* 0xC0,0x09 */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES128-SHA",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha_composite, &s2n_record_alg_aes128_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes128_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_256_cbc_sha = /* 0xC0,0x0A */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES256-SHA",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha_composite, &s2n_record_alg_aes256_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes256_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_rc4_128_sha = /* 0xC0,0x11 */ {
    .available = 0,
    .name = "ECDHE-RSA-RC4-SHA",
    .iana_value = { TLS_ECDHE_RSA_WITH_RC4_128_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_rc4_sha },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_rc4_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_3des_ede_cbc_sha = /* 0xC0,0x12 */ {
    .available = 0,
    .name = "ECDHE-RSA-DES-CBC3-SHA",
    .iana_value = { TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_3des_sha },
    .num_record_algs = 1,
    .sslv3_record_alg = &s2n_record_alg_3des_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_128_cbc_sha = /* 0xC0,0x13 */ {
    .available = 0,
    .name = "ECDHE-RSA-AES128-SHA",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha_composite, &s2n_record_alg_aes128_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes128_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_256_cbc_sha = /* 0xC0,0x14 */ {
    .available = 0,
    .name = "ECDHE-RSA-AES256-SHA",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha_composite, &s2n_record_alg_aes256_sha },
    .num_record_algs = 2,
    .sslv3_record_alg = &s2n_record_alg_aes256_sslv3_sha,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_SSLv3,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256 = /* 0xC0,0x23 */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES128-SHA256",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha256_composite, &s2n_record_alg_aes128_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384 = /* 0xC0,0x24 */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES256-SHA384",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha384 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_128_cbc_sha256 = /* 0xC0,0x27 */ {
    .available = 0,
    .name = "ECDHE-RSA-AES128-SHA256",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_sha256_composite, &s2n_record_alg_aes128_sha256 },
    .num_record_algs = 2,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_256_cbc_sha384 = /* 0xC0,0x28 */ {
    .available = 0,
    .name = "ECDHE-RSA-AES256-SHA384",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_sha384 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256 = /* 0xC0,0x2B */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES128-GCM-SHA256",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384 = /* 0xC0,0x2C */ {
    .available = 0,
    .name = "ECDHE-ECDSA-AES256-GCM-SHA384",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_128_gcm_sha256 = /* 0xC0,0x2F */ {
    .available = 0,
    .name = "ECDHE-RSA-AES128-GCM-SHA256",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes128_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_aes_256_gcm_sha384 = /* 0xC0,0x30 */ {
    .available = 0,
    .name = "ECDHE-RSA-AES256-GCM-SHA384",
    .iana_value = { TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_rsa_with_chacha20_poly1305_sha256 = /* 0xCC,0xA8 */ {
    .available = 0,
    .name = "ECDHE-RSA-CHACHA20-POLY1305",
    .iana_value = { TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_chacha20_poly1305 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_ecdhe_ecdsa_with_chacha20_poly1305_sha256 = /* 0xCC,0xA9 */ {
    .available = 0,
    .name = "ECDHE-ECDSA-CHACHA20-POLY1305",
    .iana_value = { TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 },
    .key_exchange_alg = &s2n_ecdhe,
    .auth_method = S2N_AUTHENTICATION_ECDSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_chacha20_poly1305 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_dhe_rsa_with_chacha20_poly1305_sha256 = /* 0xCC,0xAA */ {
    .available = 0,
    .name = "DHE-RSA-CHACHA20-POLY1305",
    .iana_value = { TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256 },
    .key_exchange_alg = &s2n_dhe,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_chacha20_poly1305 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS12,
};

/* From https://tools.ietf.org/html/draft-campagna-tls-bike-sike-hybrid */

struct s2n_cipher_suite s2n_ecdhe_kyber_rsa_with_aes_256_gcm_sha384 = /* 0xFF, 0x0C */ {
    .available = 0,
    .name = "ECDHE-KYBER-RSA-AES256-GCM-SHA384",
    .iana_value = { TLS_ECDHE_KYBER_RSA_WITH_AES_256_GCM_SHA384 },
    .key_exchange_alg = &s2n_hybrid_ecdhe_kem,
    .auth_method = S2N_AUTHENTICATION_RSA,
    .record_alg = NULL,
    .all_record_algs = { &s2n_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS12,
};

struct s2n_cipher_suite s2n_tls13_aes_128_gcm_sha256 = {
    .available = 0,
    .name = "TLS_AES_128_GCM_SHA256",
    .iana_value = { TLS_AES_128_GCM_SHA256 },
    .key_exchange_alg = NULL,
    .auth_method = S2N_AUTHENTICATION_METHOD_TLS13,
    .record_alg = NULL,
    .all_record_algs = { &s2n_tls13_record_alg_aes128_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS13,
};

struct s2n_cipher_suite s2n_tls13_aes_256_gcm_sha384 = {
    .available = 0,
    .name = "TLS_AES_256_GCM_SHA384",
    .iana_value = { TLS_AES_256_GCM_SHA384 },
    .key_exchange_alg = NULL,
    .auth_method = S2N_AUTHENTICATION_METHOD_TLS13,
    .record_alg = NULL,
    .all_record_algs = { &s2n_tls13_record_alg_aes256_gcm },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA384,
    .minimum_required_tls_version = S2N_TLS13,
};

struct s2n_cipher_suite s2n_tls13_chacha20_poly1305_sha256 = {
    .available = 0,
    .name = "TLS_CHACHA20_POLY1305_SHA256",
    .iana_value = { TLS_CHACHA20_POLY1305_SHA256 },
    .key_exchange_alg = NULL,
    .auth_method = S2N_AUTHENTICATION_METHOD_TLS13,
    .record_alg = NULL,
    .all_record_algs = { &s2n_tls13_record_alg_chacha20_poly1305 },
    .num_record_algs = 1,
    .sslv3_record_alg = NULL,
    .prf_alg = S2N_HMAC_SHA256,
    .minimum_required_tls_version = S2N_TLS13,
};

/* All of the cipher suites that s2n negotiates in order of IANA value.
 * New cipher suites MUST be added here, IN ORDER, or they will not be
 * properly initialized.
 */
static struct s2n_cipher_suite *s2n_all_cipher_suites[] = {
    &s2n_rsa_with_rc4_128_md5,                      /* 0x00,0x04 */
    &s2n_rsa_with_rc4_128_sha,                      /* 0x00,0x05 */
    &s2n_rsa_with_3des_ede_cbc_sha,                 /* 0x00,0x0A */
    &s2n_dhe_rsa_with_3des_ede_cbc_sha,             /* 0x00,0x16 */
    &s2n_rsa_with_aes_128_cbc_sha,                  /* 0x00,0x2F */
    &s2n_dhe_rsa_with_aes_128_cbc_sha,              /* 0x00,0x33 */
    &s2n_rsa_with_aes_256_cbc_sha,                  /* 0x00,0x35 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha,              /* 0x00,0x39 */
    &s2n_rsa_with_aes_128_cbc_sha256,               /* 0x00,0x3C */
    &s2n_rsa_with_aes_256_cbc_sha256,               /* 0x00,0x3D */
    &s2n_dhe_rsa_with_aes_128_cbc_sha256,           /* 0x00,0x67 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha256,           /* 0x00,0x6B */
    &s2n_rsa_with_aes_128_gcm_sha256,               /* 0x00,0x9C */
    &s2n_rsa_with_aes_256_gcm_sha384,               /* 0x00,0x9D */
    &s2n_dhe_rsa_with_aes_128_gcm_sha256,           /* 0x00,0x9E */
    &s2n_dhe_rsa_with_aes_256_gcm_sha384,           /* 0x00,0x9F */

    &s2n_tls13_aes_128_gcm_sha256,                  /* 0x13,0x01 */
    &s2n_tls13_aes_256_gcm_sha384,                  /* 0x13,0x02 */
    &s2n_tls13_chacha20_poly1305_sha256,            /* 0x13,0x03 */

    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha,          /* 0xC0,0x09 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha,          /* 0xC0,0x0A */
    &s2n_ecdhe_rsa_with_rc4_128_sha,                /* 0xC0,0x11 */
    &s2n_ecdhe_rsa_with_3des_ede_cbc_sha,           /* 0xC0,0x12 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha,            /* 0xC0,0x13 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha,            /* 0xC0,0x14 */
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256,       /* 0xC0,0x23 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384,       /* 0xC0,0x24 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha256,         /* 0xC0,0x27 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha384,         /* 0xC0,0x28 */
    &s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256,       /* 0xC0,0x2B */
    &s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384,       /* 0xC0,0x2C */
    &s2n_ecdhe_rsa_with_aes_128_gcm_sha256,         /* 0xC0,0x2F */
    &s2n_ecdhe_rsa_with_aes_256_gcm_sha384,         /* 0xC0,0x30 */
    &s2n_ecdhe_rsa_with_chacha20_poly1305_sha256,   /* 0xCC,0xA8 */
    &s2n_ecdhe_ecdsa_with_chacha20_poly1305_sha256, /* 0xCC,0xA9 */
    &s2n_dhe_rsa_with_chacha20_poly1305_sha256,     /* 0xCC,0xAA */
    &s2n_ecdhe_kyber_rsa_with_aes_256_gcm_sha384,   /* 0xFF,0x0C */
};

/* All supported ciphers. Exposed for integration testing. */
const struct s2n_cipher_preferences cipher_preferences_test_all = {
    .count = s2n_array_len(s2n_all_cipher_suites),
    .suites = s2n_all_cipher_suites,
};

/* All TLS12 Cipher Suites */

static struct s2n_cipher_suite *s2n_all_tls12_cipher_suites[] = {
    &s2n_rsa_with_rc4_128_md5,                      /* 0x00,0x04 */
    &s2n_rsa_with_rc4_128_sha,                      /* 0x00,0x05 */
    &s2n_rsa_with_3des_ede_cbc_sha,                 /* 0x00,0x0A */
    &s2n_dhe_rsa_with_3des_ede_cbc_sha,             /* 0x00,0x16 */
    &s2n_rsa_with_aes_128_cbc_sha,                  /* 0x00,0x2F */
    &s2n_dhe_rsa_with_aes_128_cbc_sha,              /* 0x00,0x33 */
    &s2n_rsa_with_aes_256_cbc_sha,                  /* 0x00,0x35 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha,              /* 0x00,0x39 */
    &s2n_rsa_with_aes_128_cbc_sha256,               /* 0x00,0x3C */
    &s2n_rsa_with_aes_256_cbc_sha256,               /* 0x00,0x3D */
    &s2n_dhe_rsa_with_aes_128_cbc_sha256,           /* 0x00,0x67 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha256,           /* 0x00,0x6B */
    &s2n_rsa_with_aes_128_gcm_sha256,               /* 0x00,0x9C */
    &s2n_rsa_with_aes_256_gcm_sha384,               /* 0x00,0x9D */
    &s2n_dhe_rsa_with_aes_128_gcm_sha256,           /* 0x00,0x9E */
    &s2n_dhe_rsa_with_aes_256_gcm_sha384,           /* 0x00,0x9F */

    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha,          /* 0xC0,0x09 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha,          /* 0xC0,0x0A */
    &s2n_ecdhe_rsa_with_rc4_128_sha,                /* 0xC0,0x11 */
    &s2n_ecdhe_rsa_with_3des_ede_cbc_sha,           /* 0xC0,0x12 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha,            /* 0xC0,0x13 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha,            /* 0xC0,0x14 */
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256,       /* 0xC0,0x23 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384,       /* 0xC0,0x24 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha256,         /* 0xC0,0x27 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha384,         /* 0xC0,0x28 */
    &s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256,       /* 0xC0,0x2B */
    &s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384,       /* 0xC0,0x2C */
    &s2n_ecdhe_rsa_with_aes_128_gcm_sha256,         /* 0xC0,0x2F */
    &s2n_ecdhe_rsa_with_aes_256_gcm_sha384,         /* 0xC0,0x30 */
    &s2n_ecdhe_rsa_with_chacha20_poly1305_sha256,   /* 0xCC,0xA8 */
    &s2n_ecdhe_ecdsa_with_chacha20_poly1305_sha256, /* 0xCC,0xA9 */
    &s2n_dhe_rsa_with_chacha20_poly1305_sha256,     /* 0xCC,0xAA */
    &s2n_ecdhe_kyber_rsa_with_aes_256_gcm_sha384,   /* 0xFF,0x0C */
};

const struct s2n_cipher_preferences cipher_preferences_test_all_tls12 = {
    .count = s2n_array_len(s2n_all_tls12_cipher_suites),
    .suites = s2n_all_tls12_cipher_suites,
};

/* All of the cipher suites that s2n can negotiate when in FIPS mode,
 * in order of IANA value. Exposed for the "test_all_fips" cipher preference list.
 */
static struct s2n_cipher_suite *s2n_all_fips_cipher_suites[] = {
    &s2n_rsa_with_3des_ede_cbc_sha,                /* 0x00,0x0A */
    &s2n_rsa_with_aes_128_cbc_sha,                 /* 0x00,0x2F */
    &s2n_rsa_with_aes_256_cbc_sha,                 /* 0x00,0x35 */
    &s2n_rsa_with_aes_128_cbc_sha256,              /* 0x00,0x3C */
    &s2n_rsa_with_aes_256_cbc_sha256,              /* 0x00,0x3D */
    &s2n_dhe_rsa_with_aes_128_cbc_sha256,          /* 0x00,0x67 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha256,          /* 0x00,0x6B */
    &s2n_rsa_with_aes_128_gcm_sha256,              /* 0x00,0x9C */
    &s2n_rsa_with_aes_256_gcm_sha384,              /* 0x00,0x9D */
    &s2n_dhe_rsa_with_aes_128_gcm_sha256,          /* 0x00,0x9E */
    &s2n_dhe_rsa_with_aes_256_gcm_sha384,          /* 0x00,0x9F */
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256,      /* 0xC0,0x23 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384,      /* 0xC0,0x24 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha256,        /* 0xC0,0x27 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha384,        /* 0xC0,0x28 */
    &s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256,      /* 0xC0,0x2B */
    &s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384,      /* 0xC0,0x2C */
    &s2n_ecdhe_rsa_with_aes_128_gcm_sha256,        /* 0xC0,0x2F */
    &s2n_ecdhe_rsa_with_aes_256_gcm_sha384,        /* 0xC0,0x30 */
};

/* All supported FIPS ciphers. Exposed for integration testing. */
const struct s2n_cipher_preferences cipher_preferences_test_all_fips = {
    .count = s2n_array_len(s2n_all_fips_cipher_suites),
    .suites = s2n_all_fips_cipher_suites,
};

/* All of the ECDSA cipher suites that s2n can negotiate, in order of IANA
 * value. Exposed for the "test_all_ecdsa" cipher preference list.
 */
static struct s2n_cipher_suite *s2n_all_ecdsa_cipher_suites[] = {
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha,          /* 0xC0,0x09 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha,          /* 0xC0,0x0A */
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256,       /* 0xC0,0x23 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384,       /* 0xC0,0x24 */
    &s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256,       /* 0xC0,0x2B */
    &s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384,       /* 0xC0,0x2C */
    &s2n_ecdhe_ecdsa_with_chacha20_poly1305_sha256, /* 0xCC,0xA9 */
};

/* All supported ECDSA cipher suites. Exposed for integration testing. */
const struct s2n_cipher_preferences cipher_preferences_test_all_ecdsa = {
    .count = s2n_array_len(s2n_all_ecdsa_cipher_suites),
    .suites = s2n_all_ecdsa_cipher_suites,
};

/* All cipher suites that uses RSA key exchange. Exposed for unit or integration tests. */
static struct s2n_cipher_suite *s2n_all_rsa_kex_cipher_suites[] = {
    &s2n_rsa_with_aes_128_cbc_sha,                  /* 0x00,0x2F */
    &s2n_rsa_with_rc4_128_md5,                      /* 0x00,0x04 */
    &s2n_rsa_with_rc4_128_sha,                      /* 0x00,0x05 */
    &s2n_rsa_with_3des_ede_cbc_sha,                 /* 0x00,0x0A */
    &s2n_rsa_with_aes_128_cbc_sha,                  /* 0x00,0x2F */
    &s2n_rsa_with_aes_256_cbc_sha,                  /* 0x00,0x35 */
    &s2n_rsa_with_aes_128_cbc_sha256,               /* 0x00,0x3C */
    &s2n_rsa_with_aes_256_cbc_sha256,               /* 0x00,0x3D */
    &s2n_rsa_with_aes_128_gcm_sha256,               /* 0x00,0x9C */
    &s2n_rsa_with_aes_256_gcm_sha384,               /* 0x00,0x9D */
};

/* Cipher preferences with rsa key exchange. Exposed for unit and integration tests. */
const struct s2n_cipher_preferences cipher_preferences_test_all_rsa_kex = {
    .count = s2n_array_len(s2n_all_rsa_kex_cipher_suites),
    .suites = s2n_all_rsa_kex_cipher_suites,
};

/* All ECDSA cipher suites first, then the rest of the supported ciphers that s2n can negotiate.
 * Exposed for the "test_ecdsa_priority" cipher preference list.
 */
static struct s2n_cipher_suite *s2n_ecdsa_priority_cipher_suites[] = {
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha,          /* 0xC0,0x09 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha,          /* 0xC0,0x0A */
    &s2n_ecdhe_ecdsa_with_aes_128_cbc_sha256,       /* 0xC0,0x23 */
    &s2n_ecdhe_ecdsa_with_aes_256_cbc_sha384,       /* 0xC0,0x24 */
    &s2n_ecdhe_ecdsa_with_aes_128_gcm_sha256,       /* 0xC0,0x2B */
    &s2n_ecdhe_ecdsa_with_aes_256_gcm_sha384,       /* 0xC0,0x2C */
    &s2n_ecdhe_ecdsa_with_chacha20_poly1305_sha256, /* 0xCC,0xA9 */
    &s2n_rsa_with_rc4_128_md5,                      /* 0x00,0x04 */
    &s2n_rsa_with_rc4_128_sha,                      /* 0x00,0x05 */
    &s2n_rsa_with_3des_ede_cbc_sha,                 /* 0x00,0x0A */
    &s2n_dhe_rsa_with_3des_ede_cbc_sha,             /* 0x00,0x16 */
    &s2n_rsa_with_aes_128_cbc_sha,                  /* 0x00,0x2F */
    &s2n_dhe_rsa_with_aes_128_cbc_sha,              /* 0x00,0x33 */
    &s2n_rsa_with_aes_256_cbc_sha,                  /* 0x00,0x35 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha,              /* 0x00,0x39 */
    &s2n_rsa_with_aes_128_cbc_sha256,               /* 0x00,0x3C */
    &s2n_rsa_with_aes_256_cbc_sha256,               /* 0x00,0x3D */
    &s2n_dhe_rsa_with_aes_128_cbc_sha256,           /* 0x00,0x67 */
    &s2n_dhe_rsa_with_aes_256_cbc_sha256,           /* 0x00,0x6B */
    &s2n_rsa_with_aes_128_gcm_sha256,               /* 0x00,0x9C */
    &s2n_rsa_with_aes_256_gcm_sha384,               /* 0x00,0x9D */
    &s2n_dhe_rsa_with_aes_128_gcm_sha256,           /* 0x00,0x9E */
    &s2n_dhe_rsa_with_aes_256_gcm_sha384,           /* 0x00,0x9F */
    &s2n_ecdhe_rsa_with_rc4_128_sha,                /* 0xC0,0x11 */
    &s2n_ecdhe_rsa_with_3des_ede_cbc_sha,           /* 0xC0,0x12 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha,            /* 0xC0,0x13 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha,            /* 0xC0,0x14 */
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha256,         /* 0xC0,0x27 */
    &s2n_ecdhe_rsa_with_aes_256_cbc_sha384,         /* 0xC0,0x28 */
    &s2n_ecdhe_rsa_with_aes_128_gcm_sha256,         /* 0xC0,0x2F */
    &s2n_ecdhe_rsa_with_aes_256_gcm_sha384,         /* 0xC0,0x30 */
    &s2n_ecdhe_rsa_with_chacha20_poly1305_sha256,   /* 0xCC,0xA8 */
    &s2n_dhe_rsa_with_chacha20_poly1305_sha256,     /* 0xCC,0xAA */
};

/* All cipher suites, but with ECDSA priority. Exposed for integration testing. */
const struct s2n_cipher_preferences cipher_preferences_test_ecdsa_priority = {
    .count = s2n_array_len(s2n_ecdsa_priority_cipher_suites),
    .suites = s2n_ecdsa_priority_cipher_suites,
};

static struct s2n_cipher_suite *s2n_all_tls13_cipher_suites[] = {
    &s2n_tls13_aes_128_gcm_sha256,                  /* 0x13,0x01 */
    &s2n_tls13_aes_256_gcm_sha384,                  /* 0x13,0x02 */
    &s2n_tls13_chacha20_poly1305_sha256,            /* 0x13,0x03 */
};

const struct s2n_cipher_preferences cipher_preferences_test_all_tls13 = {
    .count = s2n_array_len(s2n_all_tls13_cipher_suites),
    .suites = s2n_all_tls13_cipher_suites,
};

static struct s2n_cipher_suite *s2n_all_tls13_cipher_suites_equal_preference[] = {
    &s2n_equal_preference_group_start,              /* start group */
    &s2n_tls13_aes_128_gcm_sha256,                  /* 0x13,0x01 */
    &s2n_tls13_aes_256_gcm_sha384,                  /* 0x13,0x02 */
    &s2n_tls13_chacha20_poly1305_sha256,            /* 0x13,0x03 */
    &s2n_equal_preference_group_end,                /* end group */
};

const struct s2n_cipher_preferences cipher_preferences_test_all_equal_preference_tls13 = {
    .count = s2n_array_len(s2n_all_tls13_cipher_suites_equal_preference),
    .suites = s2n_all_tls13_cipher_suites_equal_preference,
};

/* An arbitrarily complex cipher suite w/ equal preferncing for testing purposes only */
static struct s2n_cipher_suite *s2n_test_arbitrary_equal_preference[] = {
    &s2n_ecdhe_rsa_with_aes_128_cbc_sha256,         /* 0xC0,0x27 */
    &s2n_tls13_chacha20_poly1305_sha256,            /* 0x13,0x03 */
    &s2n_equal_preference_group_start,              /* start group */
    &s2n_tls13_aes_128_gcm_sha256,                  /* 0x13,0x01 */
    &s2n_tls13_aes_256_gcm_sha384,                  /* 0x13,0x02 */
    &s2n_rsa_with_rc4_128_md5,                      /* 0x00,0x04 */
    &s2n_equal_preference_group_end,                /* end group */
    &s2n_ecdhe_rsa_with_chacha20_poly1305_sha256,   /* 0xCC,0xA8 */
};

const struct s2n_cipher_preferences cipher_preferences_test_arbitrary_equal_preferences_tls13 = {
    .count = s2n_array_len(s2n_test_arbitrary_equal_preference),
    .suites = s2n_test_arbitrary_equal_preference,
};

static bool should_init_crypto = true;
static bool crypto_initialized = false;
int s2n_crypto_disable_init(void) {
    POSIX_ENSURE(!crypto_initialized, S2N_ERR_INITIALIZED);
    should_init_crypto = false;
    return S2N_SUCCESS;
}

/* Determines cipher suite availability and selects record algorithms */
int s2n_cipher_suites_init(void)
{
    const int num_cipher_suites = s2n_array_len(s2n_all_cipher_suites);
    for (int i = 0; i < num_cipher_suites; i++) {
        struct s2n_cipher_suite *cur_suite = s2n_all_cipher_suites[i];
        cur_suite->available = 0;
        cur_suite->record_alg = NULL;

        /* Find the highest priority supported record algorithm */
        for (int j = 0; j < cur_suite->num_record_algs; j++) {
            /* Can we use the record algorithm's cipher? Won't be available if the system CPU architecture
             * doesn't support it or if the libcrypto lacks the feature. All hmac_algs are supported.
             */
            if (cur_suite->all_record_algs[j]->cipher->is_available()) {
                /* Found a supported record algorithm. Use it. */
                cur_suite->available = 1;
                cur_suite->record_alg = cur_suite->all_record_algs[j];
                break;
            }
        }

        /* Mark PQ cipher suites as unavailable if PQ is disabled */
        if (s2n_kex_includes(cur_suite->key_exchange_alg, &s2n_kem) && !s2n_pq_is_enabled()) {
            cur_suite->available = 0;
            cur_suite->record_alg = NULL;
        }

        /* Initialize SSLv3 cipher suite if SSLv3 utilizes a different record algorithm */
        if (cur_suite->sslv3_record_alg && cur_suite->sslv3_record_alg->cipher->is_available()) {
            struct s2n_blob cur_suite_mem = { .data = (uint8_t *) cur_suite, .size = sizeof(struct s2n_cipher_suite) };
            struct s2n_blob new_suite_mem = { 0 };
            POSIX_GUARD(s2n_dup(&cur_suite_mem, &new_suite_mem));

            struct s2n_cipher_suite *new_suite = (struct s2n_cipher_suite *)(void *)new_suite_mem.data;
            new_suite->available = 1;
            new_suite->record_alg = cur_suite->sslv3_record_alg;
            cur_suite->sslv3_cipher_suite = new_suite;
        } else {
            cur_suite->sslv3_cipher_suite = cur_suite;
        }
    }

    if (should_init_crypto) {
#if !S2N_OPENSSL_VERSION_AT_LEAST(1, 1, 0)
        /*https://wiki.openssl.org/index.php/Manual:OpenSSL_add_all_algorithms(3)*/
        OpenSSL_add_all_algorithms();
#else
        OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
#endif
    }

    crypto_initialized = true;

    return S2N_SUCCESS;
}

/* Reset any selected record algorithms */
int s2n_cipher_suites_cleanup(void)
{
    const int num_cipher_suites = sizeof(s2n_all_cipher_suites) / sizeof(struct s2n_cipher_suite *);
    for (int i = 0; i < num_cipher_suites; i++) {
        struct s2n_cipher_suite *cur_suite = s2n_all_cipher_suites[i];
        cur_suite->available = 0;
        cur_suite->record_alg = NULL;

        /* Release custom SSLv3 cipher suites */
        if (cur_suite->sslv3_cipher_suite != cur_suite) {
            POSIX_GUARD(s2n_free_object((uint8_t **)&cur_suite->sslv3_cipher_suite, sizeof(struct s2n_cipher_suite)));
        }
        cur_suite->sslv3_cipher_suite = NULL;
    }

    if (should_init_crypto) {
#if !S2N_OPENSSL_VERSION_AT_LEAST(1, 1, 0)
        /*https://wiki.openssl.org/index.php/Manual:OpenSSL_add_all_algorithms(3)*/
        EVP_cleanup();

        /* per the reqs here https://www.openssl.org/docs/man1.1.0/crypto/OPENSSL_init_crypto.html we don't explicitly call
        * cleanup in later versions */
#endif
    }

    return 0;
}

S2N_RESULT s2n_cipher_suite_from_iana(const uint8_t iana[static S2N_TLS_CIPHER_SUITE_LEN], struct s2n_cipher_suite **cipher_suite)
{
    RESULT_ENSURE_REF(cipher_suite);
    *cipher_suite = NULL;
    RESULT_ENSURE_REF(iana);

    int low = 0;
    int top = s2n_array_len(s2n_all_cipher_suites) - 1;

    /* Perform a textbook binary search */
    while (low <= top) {
        /* Check in the middle */
        size_t mid = low + ((top - low) / 2);
        int m = memcmp(s2n_all_cipher_suites[mid]->iana_value, iana, S2N_TLS_CIPHER_SUITE_LEN);

        if (m == 0) {
            *cipher_suite = s2n_all_cipher_suites[mid];
            return S2N_RESULT_OK;
        } else if (m > 0) {
            top = mid - 1;
        } else if (m < 0) {
            low = mid + 1;
        }
    }
    RESULT_BAIL(S2N_ERR_CIPHER_NOT_SUPPORTED);
}

int s2n_set_cipher_as_client(struct s2n_connection *conn, uint8_t wire[S2N_TLS_CIPHER_SUITE_LEN])
{
    POSIX_ENSURE_REF(conn);
    POSIX_ENSURE_REF(conn->secure);
    POSIX_ENSURE_REF(conn->secure->cipher_suite);

    const struct s2n_security_policy *security_policy;
    POSIX_GUARD(s2n_connection_get_security_policy(conn, &security_policy));
    POSIX_ENSURE_REF(security_policy);

    /**
     * Ensure that the wire cipher suite is contained in the security
     * policy, and thus was offered by the client.
     *
     *= https://tools.ietf.org/rfc/rfc8446#4.1.3
     *# A client which receives a
     *# cipher suite that was not offered MUST abort the handshake with an
     *# "illegal_parameter" alert.
     *
     *= https://tools.ietf.org/rfc/rfc8446#4.1.4
     *# A client which receives a cipher suite that was not offered MUST
     *# abort the handshake.
     *
     *= https://tools.ietf.org/rfc/rfc8446#4.1.4
     *# Upon receipt of a HelloRetryRequest, the client MUST check the
     *# legacy_version, legacy_session_id_echo, cipher_suite
     **/
    struct s2n_cipher_suite *cipher_suite = NULL;
    for (size_t i = 0; i < security_policy->cipher_preferences->count; i++) {
        const uint8_t *ours = security_policy->cipher_preferences->suites[i]->iana_value;
        if (memcmp(wire, ours, S2N_TLS_CIPHER_SUITE_LEN) == 0) {
            cipher_suite = security_policy->cipher_preferences->suites[i];
            break;
        }
    }
    POSIX_ENSURE(cipher_suite != NULL, S2N_ERR_CIPHER_NOT_SUPPORTED);

    POSIX_ENSURE(cipher_suite->available, S2N_ERR_CIPHER_NOT_SUPPORTED);

    /** Clients MUST verify
     *= https://tools.ietf.org/rfc/rfc8446#section-4.2.11
     *# that the server selected a cipher suite
     *# indicating a Hash associated with the PSK
     **/
    if (conn->psk_params.chosen_psk) {
        POSIX_ENSURE(cipher_suite->prf_alg == conn->psk_params.chosen_psk->hmac_alg,
                     S2N_ERR_CIPHER_NOT_SUPPORTED);
    }

    /**
     *= https://tools.ietf.org/rfc/rfc8446#4.1.4
     *# Upon receiving
     *# the ServerHello, clients MUST check that the cipher suite supplied in
     *# the ServerHello is the same as that in the HelloRetryRequest and
     *# otherwise abort the handshake with an "illegal_parameter" alert.
     **/
    if (s2n_is_hello_retry_handshake(conn) && !s2n_is_hello_retry_message(conn)) {
        POSIX_ENSURE(conn->secure->cipher_suite->iana_value == cipher_suite->iana_value, S2N_ERR_CIPHER_NOT_SUPPORTED);
        return S2N_SUCCESS;
    }

    conn->secure->cipher_suite = cipher_suite;

    /* For SSLv3 use SSLv3-specific ciphers */
    if (conn->actual_protocol_version == S2N_SSLv3) {
        conn->secure->cipher_suite = conn->secure->cipher_suite->sslv3_cipher_suite;
        POSIX_ENSURE_REF(conn->secure->cipher_suite);
    }

    return 0;
}

static int s2n_wire_ciphers_contain(const uint8_t *match, const uint8_t *wire, uint32_t count, uint32_t cipher_suite_len)
{
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *theirs = wire + (i * cipher_suite_len) + (cipher_suite_len - S2N_TLS_CIPHER_SUITE_LEN);

        if (!memcmp(match, theirs, S2N_TLS_CIPHER_SUITE_LEN)) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief While a potential cipher suite match has been identified, we still need to check if the cipher suite
 *        can actually be used for the remainder of the connection. These checks ensure that there exists an
 *        implementation, whether the versions and PSKs are compatible.
 * 
 * @param conn s2n_connection context
 * @param potential_match Potential cipher suite match
 * @return true Cipher suite match is usable
 * @return false Cipher suite match is not usable for this connection
 */
static bool s2n_cipher_suite_match_is_valid(struct s2n_connection* conn, struct s2n_cipher_suite* potential_match) {
        /* Never use TLS1.3 ciphers on a pre-TLS1.3 connection, and vice versa */
        if ((conn->actual_protocol_version >= S2N_TLS13) != (potential_match->minimum_required_tls_version >= S2N_TLS13)) {
            return false;
        }

        /* Skip the suite if we don't have an available implementation */
        if (!potential_match->available) {
            return false;
        }

        /* Make sure the cipher is valid for available certs */
        if (s2n_is_cipher_suite_valid_for_auth(conn, potential_match) != S2N_SUCCESS) {
            return false;
        }

        /* TLS 1.3 does not include key exchange in cipher suites */
        if (potential_match->minimum_required_tls_version < S2N_TLS13) {
            /* If the kex is not supported continue to the next candidate */
            bool kex_supported = false;
            POSIX_GUARD_RESULT(s2n_kex_supported(potential_match, conn, &kex_supported));
            if (!kex_supported) {
                return false;
            }

            /* If the kex is not configured correctly continue to the next candidate */
            if (s2n_result_is_error(s2n_configure_kex(potential_match, conn))) {
                return false;
            }
        }

        /**
        *= https://tools.ietf.org/rfc/rfc8446#section-4.2.11
        *# The server MUST ensure that it selects a compatible PSK
        *# (if any) and cipher suite.
        **/
        if (conn->psk_params.chosen_psk != NULL) {
            if (potential_match->prf_alg != conn->psk_params.chosen_psk->hmac_alg) {
                return false;
            }
        }

        return true;
}

/**
 * @brief Same as s2n_wire_ciphers_contain() except that it returns the match's index in the client wire.
 * 
 * @param match Cipher suite IANA which we wish to find a match for
 * @param wire Client wire
 * @param count Number of entries
 * @param cipher_suite_len Length offset
 * @return int Index of the match in the client wire. If a match was not found then -1 is returned.
 */
static int s2n_wire_ciphers_has_server_cipher_at(const uint8_t *match, const uint8_t *wire, uint32_t count, uint32_t cipher_suite_len)
{
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *theirs = wire + (i * cipher_suite_len) + (cipher_suite_len - S2N_TLS_CIPHER_SUITE_LEN);

        if (!memcmp(match, theirs, S2N_TLS_CIPHER_SUITE_LEN)) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief This is the logic for parsing a list of s2n_cipher_suites* and selecting a cipher_suite accounting
 *        for equal preference grouping. This method assumes that the server side cipher suite preference is
 *        correctly formatted eg for every group_start delimiter, there exists a following group_end delimiter.
 *        TODO: Add reference to documentation and add basic validation
 * 
 * @param conn s2n_connection is used to ensure the negotiated cipher is valid for the connection
 * @param wire client wire containing cipher suite IANAs
 * @param count Number of entries
 * @param cipher_suite_len Cipher suite length offset
 * @return int Server's index of the negotiated cipher suite. If no cipher suite was selected then -1 is returned.
 */
static int s2n_get_negotiated_server_index(struct s2n_connection* conn,
                                           const uint8_t* wire,
                                           uint32_t count,
                                           uint32_t cipher_suite_len) {
    POSIX_ENSURE_REF(wire);
    POSIX_ENSURE_REF(conn);

    const struct s2n_security_policy *security_policy;
    POSIX_GUARD(s2n_connection_get_security_policy(conn, &security_policy));
    const struct s2n_cipher_preferences* cipher_preferences = security_policy->cipher_preferences;
    POSIX_ENSURE_REF(cipher_preferences);

    bool in_group = false;
    int negotiated_client_index = count;
    int negotiated_server_index = -1, negotiated_server_highest_vers_match_index = -1;

    for (uint32_t i = 0; i < cipher_preferences->count; i++) {
        const struct s2n_cipher_suite* ours = cipher_preferences->suites[i];
        /* Check if the cipher suite is an equal-preference grouping delimiter */
        if (ours == &s2n_equal_preference_group_start) {
            in_group = true;
            continue;
        } 
        if (ours == &s2n_equal_preference_group_end) {
            in_group = false;
            /* Exiting a group and a negotiated cipher has already been found. */
            if (negotiated_server_index != -1) {
                return negotiated_server_index;
            }
            continue;
        }

        /* Cipher suite is NOT a delimiter */
        int client_index = s2n_wire_ciphers_has_server_cipher_at(ours->iana_value, wire, count, cipher_suite_len);

        /* Client does not support this cipher. Skip. */
        if (client_index < 0) {
            continue;
        }

        /* Found a potential match. Validate that the cipher suite is suitable for this connection. */
        struct s2n_cipher_suite* match = security_policy->cipher_preferences->suites[i];
        if (!s2n_cipher_suite_match_is_valid(conn, match)) {
            continue;
        }

        /* Don't immediately choose a cipher the connection shouldn't be able to support */
        if (conn->actual_protocol_version < match->minimum_required_tls_version) {
            if (negotiated_server_highest_vers_match_index == -1) {
                negotiated_server_highest_vers_match_index = i;
            }
            continue;
        }
        if (in_group) {
            /* Both client and server support a grouped-cipher */
            if (client_index < negotiated_client_index) {
                negotiated_client_index = client_index;
                negotiated_server_index = i;
            }
        } else {
            /* Else, the client and server both support a non-grouped cipher. */
            negotiated_server_index = i;
            break;
        }
    }

    /* Settle for a cipher with a higher required proto version, if it was set */
    if (negotiated_server_index == -1 && negotiated_server_highest_vers_match_index != -1) {
        return negotiated_server_highest_vers_match_index;
    }

    return negotiated_server_index;
}

static int s2n_set_cipher_as_server(struct s2n_connection *conn, uint8_t *wire, uint32_t count, uint32_t cipher_suite_len)
{
    POSIX_ENSURE_REF(conn);
    POSIX_ENSURE_REF(conn->secure);

    uint8_t renegotiation_info_scsv[S2N_TLS_CIPHER_SUITE_LEN] = { TLS_EMPTY_RENEGOTIATION_INFO_SCSV };

    /* RFC 7507 - If client is attempting to negotiate a TLS Version that is lower than the highest supported server
     * version, and the client cipher list contains TLS_FALLBACK_SCSV, then the server must abort the connection since
     * TLS_FALLBACK_SCSV should only be present when the client previously failed to negotiate a higher TLS version.
     */
    if (conn->client_protocol_version < conn->server_protocol_version) {
        uint8_t fallback_scsv[S2N_TLS_CIPHER_SUITE_LEN] = { TLS_FALLBACK_SCSV };
        if (s2n_wire_ciphers_contain(fallback_scsv, wire, count, cipher_suite_len)) {
            conn->closed = 1;
            POSIX_BAIL(S2N_ERR_FALLBACK_DETECTED);
        }
    }

    /* RFC5746 Section 3.6: A server must check if TLS_EMPTY_RENEGOTIATION_INFO_SCSV is included */
    if (s2n_wire_ciphers_contain(renegotiation_info_scsv, wire, count, cipher_suite_len)) {
        conn->secure_renegotiation = 1;
    }

    const struct s2n_security_policy *security_policy;
    POSIX_GUARD(s2n_connection_get_security_policy(conn, &security_policy));

    /* Determine the index for the negotiated cipher suite. Index is a server cipher prefernece index. */
    int negotiated_index = s2n_get_negotiated_server_index(conn, wire, count, cipher_suite_len);
    if (negotiated_index < 0) {
        POSIX_BAIL(S2N_ERR_CIPHER_NOT_SUPPORTED);
    }

    struct s2n_cipher_suite* match = security_policy->cipher_preferences->suites[negotiated_index];
    POSIX_ENSURE_REF(match);
    conn->secure->cipher_suite = match;
    return S2N_SUCCESS;
}

int s2n_set_cipher_as_sslv2_server(struct s2n_connection *conn, uint8_t *wire, uint16_t count)
{
    return s2n_set_cipher_as_server(conn, wire, count, S2N_SSLv2_CIPHER_SUITE_LEN);
}

int s2n_set_cipher_as_tls_server(struct s2n_connection *conn, uint8_t *wire, uint16_t count)
{
    return s2n_set_cipher_as_server(conn, wire, count, S2N_TLS_CIPHER_SUITE_LEN);
}

bool s2n_cipher_suite_requires_ecc_extension(struct s2n_cipher_suite *cipher)
{
    if(!cipher) {
        return false;
    }

    /* TLS1.3 does not include key exchange algorithms in its cipher suites,
     * but the elliptic curves extension is always required. */
    if (cipher->minimum_required_tls_version >= S2N_TLS13) {
        return true;
    }

    if (s2n_kex_includes(cipher->key_exchange_alg, &s2n_ecdhe)) {
        return true;
    }

    return false;
}

bool s2n_cipher_suite_requires_pq_extension(struct s2n_cipher_suite *cipher)
{
    if(!cipher) {
        return false;
    }

    if (s2n_kex_includes(cipher->key_exchange_alg, &s2n_kem)) {
        return true;
    }
    return false;
}
