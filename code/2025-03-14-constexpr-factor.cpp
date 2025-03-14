#include <cassert>
#include <cstdint>
#include <vector>

#ifdef _MSC_VER
#include <bit>
#include <__msvc_int128.hpp>
using uint128 = std::_Unsigned128;
#define __builtin_ctzll(x) std::countr_zero(x)
#else
using uint128 = unsigned __int128;
#endif

static_assert(sizeof(uintmax_t) == 8);
static_assert(uintmax_t(-1) >> 63 == 1);

/* 2*3*5*7*11...*101 is 128 bits, and has 26 prime factors */
#define MAX_NFACTS 26

#define PRIMES_H_CONTENTS \
P(1, 26, (((((uintmax_t) 0xaaaaU << 28 | 0xaaaaaaaU) << 28 | 0xaaaaaaaU) << 28 | 0xaaaaaaaU) << 28 | 0xaaaaaabU), UINTMAX_MAX / 3) \
P(2, 26, (((((uintmax_t) 0xccccU << 28 | 0xcccccccU) << 28 | 0xcccccccU) << 28 | 0xcccccccU) << 28 | 0xccccccdU), UINTMAX_MAX / 5) \
P(2, 30, (((((uintmax_t) 0xb6dbU << 28 | 0x6db6db6U) << 28 | 0xdb6db6dU) << 28 | 0xb6db6dbU) << 28 | 0x6db6db7U), UINTMAX_MAX / 7) \
P(4, 30, (((((uintmax_t) 0xa2e8U << 28 | 0xba2e8baU) << 28 | 0x2e8ba2eU) << 28 | 0x8ba2e8bU) << 28 | 0xa2e8ba3U), UINTMAX_MAX / 11) \
P(2, 30, (((((uintmax_t) 0xc4ecU << 28 | 0x4ec4ec4U) << 28 | 0xec4ec4eU) << 28 | 0xc4ec4ecU) << 28 | 0x4ec4ec5U), UINTMAX_MAX / 13) \
P(4, 30, (((((uintmax_t) 0xf0f0U << 28 | 0xf0f0f0fU) << 28 | 0x0f0f0f0U) << 28 | 0xf0f0f0fU) << 28 | 0x0f0f0f1U), UINTMAX_MAX / 17) \
P(2, 34, (((((uintmax_t) 0xbca1U << 28 | 0xaf286bcU) << 28 | 0xa1af286U) << 28 | 0xbca1af2U) << 28 | 0x86bca1bU), UINTMAX_MAX / 19) \
P(4, 36, (((((uintmax_t) 0x4de9U << 28 | 0xbd37a6fU) << 28 | 0x4de9bd3U) << 28 | 0x7a6f4deU) << 28 | 0x9bd37a7U), UINTMAX_MAX / 23) \
P(6, 32, (((((uintmax_t) 0xc234U << 28 | 0xf72c234U) << 28 | 0xf72c234U) << 28 | 0xf72c234U) << 28 | 0xf72c235U), UINTMAX_MAX / 29) \
P(2, 36, (((((uintmax_t) 0xdef7U << 28 | 0xbdef7bdU) << 28 | 0xef7bdefU) << 28 | 0x7bdef7bU) << 28 | 0xdef7bdfU), UINTMAX_MAX / 31) \
P(6, 34, (((((uintmax_t) 0xc1baU << 28 | 0xcf914c1U) << 28 | 0xbacf914U) << 28 | 0xc1bacf9U) << 28 | 0x14c1badU), UINTMAX_MAX / 37) \
P(4, 32, (((((uintmax_t) 0x18f9U << 28 | 0xc18f9c1U) << 28 | 0x8f9c18fU) << 28 | 0x9c18f9cU) << 28 | 0x18f9c19U), UINTMAX_MAX / 41) \
P(2, 36, (((((uintmax_t) 0xbe82U << 28 | 0xfa0be82U) << 28 | 0xfa0be82U) << 28 | 0xfa0be82U) << 28 | 0xfa0be83U), UINTMAX_MAX / 43) \
P(4, 36, (((((uintmax_t) 0x3677U << 28 | 0xd46cefaU) << 28 | 0x8d9df51U) << 28 | 0xb3bea36U) << 28 | 0x77d46cfU), UINTMAX_MAX / 47) \
P(6, 36, (((((uintmax_t) 0x1352U << 28 | 0x1cfb2b7U) << 28 | 0x8c13521U) << 28 | 0xcfb2b78U) << 28 | 0xc13521dU), UINTMAX_MAX / 53) \
P(6, 38, (((((uintmax_t) 0x8f2fU << 28 | 0xba93868U) << 28 | 0x22b63cbU) << 28 | 0xeea4e1aU) << 28 | 0x08ad8f3U), UINTMAX_MAX / 59) \
P(2, 40, (((((uintmax_t) 0x14fbU << 28 | 0xcda3ac1U) << 28 | 0x0c9714fU) << 28 | 0xbcda3acU) << 28 | 0x10c9715U), UINTMAX_MAX / 61) \
P(6, 36, (((((uintmax_t) 0xc2ddU << 28 | 0x9ca81e9U) << 28 | 0x131abf0U) << 28 | 0xb7672a0U) << 28 | 0x7a44c6bU), UINTMAX_MAX / 67) \
P(4, 36, (((((uintmax_t) 0x4f52U << 28 | 0xedf8c9eU) << 28 | 0xa5dbf19U) << 28 | 0x3d4bb7eU) << 28 | 0x327a977U), UINTMAX_MAX / 71) \
P(2, 36, (((((uintmax_t) 0x3f1fU << 28 | 0x8fc7e3fU) << 28 | 0x1f8fc7eU) << 28 | 0x3f1f8fcU) << 28 | 0x7e3f1f9U), UINTMAX_MAX / 73) \
P(6, 34, (((((uintmax_t) 0xd5dfU << 28 | 0x984dc5aU) << 28 | 0xbbf309bU) << 28 | 0x8b577e6U) << 28 | 0x13716afU), UINTMAX_MAX / 79) \
P(4, 44, (((((uintmax_t) 0x2818U << 28 | 0xacb90f6U) << 28 | 0xbf3a9a3U) << 28 | 0x784a062U) << 28 | 0xb2e43dbU), UINTMAX_MAX / 83) \
P(6, 42, (((((uintmax_t) 0xd1faU << 28 | 0x3f47e8fU) << 28 | 0xd1fa3f4U) << 28 | 0x7e8fd1fU) << 28 | 0xa3f47e9U), UINTMAX_MAX / 89) \
P(8, 40, (((((uintmax_t) 0x5f02U << 28 | 0xa3a0fd5U) << 28 | 0xc5f02a3U) << 28 | 0xa0fd5c5U) << 28 | 0xf02a3a1U), UINTMAX_MAX / 97) \
P(4, 38, (((((uintmax_t) 0xc32bU << 28 | 0x16cfd77U) << 28 | 0x20f353aU) << 28 | 0x4c0a237U) << 28 | 0xc32b16dU), UINTMAX_MAX / 101) \
P(2, 46, (((((uintmax_t) 0xd0c6U << 28 | 0xd5bf60eU) << 28 | 0xe9a18daU) << 28 | 0xb7ec1ddU) << 28 | 0x3431b57U), UINTMAX_MAX / 103) \
P(4, 44, (((((uintmax_t) 0xa2b1U << 28 | 0x0bf66e0U) << 28 | 0xe5aea77U) << 28 | 0xa04c8f8U) << 28 | 0xd28ac43U), UINTMAX_MAX / 107) \
P(2, 48, (((((uintmax_t) 0xc096U << 28 | 0x4fda6c0U) << 28 | 0x964fda6U) << 28 | 0xc0964fdU) << 28 | 0xa6c0965U), UINTMAX_MAX / 109) \
P(4, 50, (((((uintmax_t) 0xc090U << 28 | 0xfdbc090U) << 28 | 0xfdbc090U) << 28 | 0xfdbc090U) << 28 | 0xfdbc091U), UINTMAX_MAX / 113) \
P(14, 40, (((((uintmax_t) 0xbf7eU << 28 | 0xfdfbf7eU) << 28 | 0xfdfbf7eU) << 28 | 0xfdfbf7eU) << 28 | 0xfdfbf7fU), UINTMAX_MAX / 127) \
P(4, 42, (((((uintmax_t) 0xf82eU << 28 | 0xe6986d6U) << 28 | 0xf63aa03U) << 28 | 0xe88cb3cU) << 28 | 0x9484e2bU), UINTMAX_MAX / 131) \
P(6, 42, (((((uintmax_t) 0x21a2U << 28 | 0x91c0779U) << 28 | 0x75b8fe2U) << 28 | 0x1a291c0U) << 28 | 0x77975b9U), UINTMAX_MAX / 137) \
P(2, 42, (((((uintmax_t) 0xa212U << 28 | 0x6ad1f4fU) << 28 | 0x31ba03aU) << 28 | 0xef6ca97U) << 28 | 0x0586723U), UINTMAX_MAX / 139) \
P(10, 42, (((((uintmax_t) 0x93c2U << 28 | 0x25cc74dU) << 28 | 0x50c06dfU) << 28 | 0x5b0f768U) << 28 | 0xce2cabdU), UINTMAX_MAX / 149) \
P(2, 42, (((((uintmax_t) 0x26feU << 28 | 0x4dfc9bfU) << 28 | 0x937f26fU) << 28 | 0xe4dfc9bU) << 28 | 0xf937f27U), UINTMAX_MAX / 151) \
P(6, 40, (((((uintmax_t) 0x0685U << 28 | 0xb4fe5e9U) << 28 | 0x2c0685bU) << 28 | 0x4fe5e92U) << 28 | 0xc0685b5U), UINTMAX_MAX / 157) \
P(6, 36, (((((uintmax_t) 0x8bc7U << 28 | 0x75ca99eU) << 28 | 0xa03241fU) << 28 | 0x693a1c4U) << 28 | 0x51ab30bU), UINTMAX_MAX / 163) \
P(4, 44, (((((uintmax_t) 0x513eU << 28 | 0xd9ad38bU) << 28 | 0x7f3bc8dU) << 28 | 0x07aa27dU) << 28 | 0xb35a717U), UINTMAX_MAX / 167) \
P(6, 50, (((((uintmax_t) 0x133cU << 28 | 0xaba736cU) << 28 | 0x05eb488U) << 28 | 0x2383b30U) << 28 | 0xd516325U), UINTMAX_MAX / 173) \
P(6, 48, (((((uintmax_t) 0x0e4dU << 28 | 0x3aa30a0U) << 28 | 0x2dc3eedU) << 28 | 0x6866f8dU) << 28 | 0x962ae7bU), UINTMAX_MAX / 179) \
P(2, 48, (((((uintmax_t) 0x6fbcU << 28 | 0x1c498c0U) << 28 | 0x5a84f34U) << 28 | 0x54dca41U) << 28 | 0x0f8ed9dU), UINTMAX_MAX / 181) \
P(10, 42, (((((uintmax_t) 0x7749U << 28 | 0xb79f7f5U) << 28 | 0x470961dU) << 28 | 0x7ca632eU) << 28 | 0xe936f3fU), UINTMAX_MAX / 191) \
P(2, 46, (((((uintmax_t) 0x9094U << 28 | 0x8f40feaU) << 28 | 0xc6f6b70U) << 28 | 0xbf01539U) << 28 | 0x0948f41U), UINTMAX_MAX / 193) \
P(4, 44, (((((uintmax_t) 0x0bb2U << 28 | 0x07cc053U) << 28 | 0x2ae21c9U) << 28 | 0x6bdb9d3U) << 28 | 0xd137e0dU), UINTMAX_MAX / 197) \
P(2, 52, (((((uintmax_t) 0x7a36U << 28 | 0x07b7f5bU) << 28 | 0x5630e26U) << 28 | 0x97cc8aeU) << 28 | 0xf46c0f7U), UINTMAX_MAX / 199) \
P(12, 46, (((((uintmax_t) 0x2f51U << 28 | 0x4a026d3U) << 28 | 0x1be7bc0U) << 28 | 0xe8f2a76U) << 28 | 0xe68575bU), UINTMAX_MAX / 211) \
P(12, 40, (((((uintmax_t) 0xdd8fU << 28 | 0x7f6d0eeU) << 28 | 0xc7bfb68U) << 28 | 0x7763dfdU) << 28 | 0xb43bb1fU), UINTMAX_MAX / 223) \
P(4, 42, (((((uintmax_t) 0x766aU << 28 | 0x024168eU) << 28 | 0x18cf81bU) << 28 | 0x10ea929U) << 28 | 0xba144cbU), UINTMAX_MAX / 227) \
P(2, 42, (((((uintmax_t) 0x0c4cU << 28 | 0x0478bbcU) << 28 | 0xecfee1dU) << 28 | 0x10c4c04U) << 28 | 0x78bbcedU), UINTMAX_MAX / 229) \
P(4, 44, (((((uintmax_t) 0x758fU << 28 | 0xee6bac7U) << 28 | 0xf735d63U) << 28 | 0xfb9aeb1U) << 28 | 0xfdcd759U), UINTMAX_MAX / 233) \
P(6, 42, (((((uintmax_t) 0x077fU << 28 | 0x76e538cU) << 28 | 0x5167e64U) << 28 | 0xafaa4f4U) << 28 | 0x37b2e0fU), UINTMAX_MAX / 239) \
P(2, 42, (((((uintmax_t) 0x10feU << 28 | 0xf010fefU) << 28 | 0x010fef0U) << 28 | 0x10fef01U) << 28 | 0x0fef011U), UINTMAX_MAX / 241) \
P(10, 42, (((((uintmax_t) 0xa020U << 28 | 0xa32fefaU) << 28 | 0xe680828U) << 28 | 0xcbfbeb9U) << 28 | 0xa020a33U), UINTMAX_MAX / 251) \
P(6, 50, (((((uintmax_t) 0xff00U << 28 | 0xff00ff0U) << 28 | 0x0ff00ffU) << 28 | 0x00ff00fU) << 28 | 0xf00ff01U), UINTMAX_MAX / 257) \
P(6, 48, (((((uintmax_t) 0xf836U << 28 | 0x826ef73U) << 28 | 0xd52bcd6U) << 28 | 0x24fd147U) << 28 | 0x0e99cb7U), UINTMAX_MAX / 263) \
P(6, 44, (((((uintmax_t) 0x3ce8U << 28 | 0x354b2eaU) << 28 | 0x1c8cd8fU) << 28 | 0xb3ddbd6U) << 28 | 0x205b5c5U), UINTMAX_MAX / 269) \
P(2, 46, (((((uintmax_t) 0x8715U << 28 | 0xba188f9U) << 28 | 0x63302d5U) << 28 | 0x7da36caU) << 28 | 0x27acdefU), UINTMAX_MAX / 271) \
P(6, 54, (((((uintmax_t) 0xb25eU << 28 | 0x4463cffU) << 28 | 0x13686eeU) << 28 | 0x70c03b2U) << 28 | 0x5e4463dU), UINTMAX_MAX / 277) \
P(4, 56, (((((uintmax_t) 0x6c69U << 28 | 0xae01d27U) << 28 | 0x2ca3fc5U) << 28 | 0xb1a6b80U) << 28 | 0x749cb29U), UINTMAX_MAX / 281) \
P(2, 64, (((((uintmax_t) 0xf26eU << 28 | 0x5c44bfcU) << 28 | 0x61b2347U) << 28 | 0x768073cU) << 28 | 0x9b97113U), UINTMAX_MAX / 283) \
P(10, 56, (((((uintmax_t) 0xb07dU << 28 | 0xd0d1b15U) << 28 | 0xd7cf125U) << 28 | 0x91e9488U) << 28 | 0x4ce32adU), UINTMAX_MAX / 293) \
P(14, 46, (((((uintmax_t) 0xd2f8U << 28 | 0x7ebfcaaU) << 28 | 0x1c5a0f0U) << 28 | 0x2806abcU) << 28 | 0x74be1fbU), UINTMAX_MAX / 307) \
P(4, 48, (((((uintmax_t) 0xbe25U << 28 | 0xdd6d7aaU) << 28 | 0x646ca7eU) << 28 | 0xc3e8f3aU) << 28 | 0x7198487U), UINTMAX_MAX / 311) \
P(2, 54, (((((uintmax_t) 0xbc1dU << 28 | 0x71afd8bU) << 28 | 0xdc03458U) << 28 | 0x550f8a3U) << 28 | 0x9409d09U), UINTMAX_MAX / 313) \
P(4, 56, (((((uintmax_t) 0x2ed6U << 28 | 0xd05a72aU) << 28 | 0xcd1f7ecU) << 28 | 0x9e48ae6U) << 28 | 0xf71de15U), UINTMAX_MAX / 317) \
P(14, 48, (((((uintmax_t) 0x62ffU << 28 | 0x3a018bfU) << 28 | 0xce8062fU) << 28 | 0xf3a018bU) << 28 | 0xfce8063U), UINTMAX_MAX / 331) \
P(6, 46, (((((uintmax_t) 0x3fcfU << 28 | 0x61fe7b0U) << 28 | 0xff3d87fU) << 28 | 0x9ec3fcfU) << 28 | 0x61fe7b1U), UINTMAX_MAX / 337) \
P(10, 42, (((((uintmax_t) 0x398bU << 28 | 0x6f668c2U) << 28 | 0xc43df89U) << 28 | 0xf5abe57U) << 28 | 0x0e046d3U), UINTMAX_MAX / 347) \
P(2, 48, (((((uintmax_t) 0x8c1aU << 28 | 0x682913cU) << 28 | 0xe1ecedaU) << 28 | 0x971b23fU) << 28 | 0x1545af5U), UINTMAX_MAX / 349) \
P(4, 48, (((((uintmax_t) 0x0b9aU << 28 | 0x7862a0fU) << 28 | 0xf465879U) << 28 | 0xd5f00b9U) << 28 | 0xa7862a1U), UINTMAX_MAX / 353) \
P(6, 50, (((((uintmax_t) 0xe7c1U << 28 | 0x3f77161U) << 28 | 0xb18f54dU) << 28 | 0xba1df32U) << 28 | 0xa128a57U), UINTMAX_MAX / 359) \
P(8, 52, (((((uintmax_t) 0x7318U << 28 | 0x6a06f9bU) << 28 | 0x8d9a287U) << 28 | 0x530217bU) << 28 | 0x7747d8fU), UINTMAX_MAX / 367) \
P(6, 48, (((((uintmax_t) 0x7c39U << 28 | 0xa6c708eU) << 28 | 0xc18b530U) << 28 | 0xbaae53bU) << 28 | 0xb5e06ddU), UINTMAX_MAX / 373) \
P(6, 52, (((((uintmax_t) 0x3763U << 28 | 0x4af9ebbU) << 28 | 0xc742deeU) << 28 | 0x70206c1U) << 28 | 0x2e9b5b3U), UINTMAX_MAX / 379) \
P(4, 50, (((((uintmax_t) 0x5035U << 28 | 0x78fb523U) << 28 | 0x6cf34cdU) << 28 | 0xde9462eU) << 28 | 0xc9dbe7fU), UINTMAX_MAX / 383) \
P(6, 50, (((((uintmax_t) 0xbcdfU << 28 | 0xc0d2975U) << 28 | 0xccab1afU) << 28 | 0xb64b05eU) << 28 | 0xc41cf4dU), UINTMAX_MAX / 389) \
P(8, 46, (((((uintmax_t) 0xf5aeU << 28 | 0xc02944fU) << 28 | 0xf5aec02U) << 28 | 0x944ff5aU) << 28 | 0xec02945U), UINTMAX_MAX / 397) \
P(4, 48, (((((uintmax_t) 0xc7d2U << 28 | 0x08f00a3U) << 28 | 0x6e71a2cU) << 28 | 0xb033128U) << 28 | 0x382df71U), UINTMAX_MAX / 401) \
P(8, 48, (((((uintmax_t) 0xd38fU << 28 | 0x55c0280U) << 28 | 0xf05a21cU) << 28 | 0xcacc0c8U) << 28 | 0x4b1c2a9U), UINTMAX_MAX / 409) \
P(10, 42, (((((uintmax_t) 0xca3bU << 28 | 0xe03aa76U) << 28 | 0x87a3219U) << 28 | 0xa93db57U) << 28 | 0x5eb3a0bU), UINTMAX_MAX / 419) \
P(2, 42, (((((uintmax_t) 0x6a69U << 28 | 0xce2344bU) << 28 | 0x66c3cceU) << 28 | 0xbeef94fU) << 28 | 0xa86fe2dU), UINTMAX_MAX / 421) \
P(10, 36, (((((uintmax_t) 0xfecfU << 28 | 0xe37d53bU) << 28 | 0xfd9fc6fU) << 28 | 0xaa77fb3U) << 28 | 0xf8df54fU), UINTMAX_MAX / 431) \
P(2, 46, (((((uintmax_t) 0xa58aU << 28 | 0xf00975aU) << 28 | 0x750ff68U) << 28 | 0xa58af00U) << 28 | 0x975a751U), UINTMAX_MAX / 433) \
P(6, 48, (((((uintmax_t) 0xdc6dU << 28 | 0xa187df5U) << 28 | 0x80dfed5U) << 28 | 0x6e36d0cU) << 28 | 0x3efac07U), UINTMAX_MAX / 439) \
P(4, 48, (((((uintmax_t) 0x8fe4U << 28 | 0x4308ab0U) << 28 | 0xd4a8bd8U) << 28 | 0xb44c47aU) << 28 | 0x8299b73U), UINTMAX_MAX / 443) \
P(6, 50, (((((uintmax_t) 0xf1bfU << 28 | 0x0091f5bU) << 28 | 0xcb8bb02U) << 28 | 0xd9ccaf9U) << 28 | 0xba70e41U), UINTMAX_MAX / 449) \
P(8, 255, (((((uintmax_t) 0x5e1cU << 28 | 0x023d9e8U) << 28 | 0x78ff709U) << 28 | 0x85e1c02U) << 28 | 0x3d9e879U), UINTMAX_MAX / 457) \
P(4, 255, (((((uintmax_t) 0x7880U << 28 | 0xd53da3dU) << 28 | 0x15a842aU) << 28 | 0x343316cU) << 28 | 0x494d305U), UINTMAX_MAX / 461) \
P(2, 255, (((((uintmax_t) 0x1ddbU << 28 | 0x81ef699U) << 28 | 0xb5e8c70U) << 28 | 0xcb7916aU) << 28 | 0xb67652fU), UINTMAX_MAX / 463) \
P(4, 255, (((((uintmax_t) 0xf364U << 28 | 0x5121706U) << 28 | 0x07acad3U) << 28 | 0x98f132fU) << 28 | 0xb10fe5bU), UINTMAX_MAX / 467) \
P(12, 255, (((((uintmax_t) 0xadb1U << 28 | 0xf8848afU) << 28 | 0x4c6d06fU) << 28 | 0x2a38a6bU) << 28 | 0xf54fa1fU), UINTMAX_MAX / 479) \
P(8, 255, (((((uintmax_t) 0xd9a0U << 28 | 0x541b55aU) << 28 | 0xf0c1721U) << 28 | 0x1df689bU) << 28 | 0x98f81d7U), UINTMAX_MAX / 487) \
P(4, 255, (((((uintmax_t) 0x673bU << 28 | 0xf592825U) << 28 | 0x8a2ac0eU) << 28 | 0x994983eU) << 28 | 0x90f1ec3U), UINTMAX_MAX / 491) \
P(8, 255, (((((uintmax_t) 0x0ddaU << 28 | 0x093c062U) << 28 | 0x8041aadU) << 28 | 0x671e44bU) << 28 | 0xed87f3bU), UINTMAX_MAX / 499)

#define FIRST_OMITTED_PRIME 503

struct factors {
  uintmax_t     plarge[2]; /* Can have a single large factor */
  uintmax_t     p[MAX_NFACTS];
  unsigned char e[MAX_NFACTS];
  unsigned char nfactors;
};

constexpr void factor(uintmax_t t0, struct factors *factors);

constexpr uintmax_t gcd_odd(uintmax_t a, uintmax_t b) {
  if ((b & 1) == 0) {
    uintmax_t t = b;
    b = a;
    a = t;
  }
  if (a == 0)
    return b;

  /* Take out least significant one bit, to make room for sign */
  b >>= 1;

  while (true) {
    a >>= __builtin_ctzll(a) + 1;
    if (a == b) {
      return (a << 1) + 1;
    } else if (a > b) {
      a = (a - b);
    } else {
      uintmax_t olda = a;
      a = (b - a);
      b = olda;
    }
  }
}

static constexpr void
factor_insert_multiplicity(struct factors *factors, uintmax_t prime, int m) {
  int nfactors = factors->nfactors;
  uintmax_t *p = factors->p;
  unsigned char *e = factors->e;

  /* Locate position for insert new or increment e.  */
  int i;
  for (i = nfactors - 1; i >= 0; --i) {
    if (p[i] <= prime)
      break;
  }

  if (i < 0 || p[i] != prime) {
    for (int j = nfactors - 1; j > i; --j) {
      p[j + 1] = p[j];
      e[j + 1] = e[j];
    }
    p[i + 1] = prime;
    e[i + 1] = m;
    factors->nfactors = nfactors + 1;
  } else {
    e[i] += m;
  }
}

static constexpr unsigned char primes_diff[] = {
#define P(a,b,c,d) a,
  PRIMES_H_CONTENTS
#undef P
  0,0,0,0,0,0,0                           /* 7 sentinels for 8-way loop */
};

#define PRIMES_PTAB_ENTRIES \
  (sizeof(primes_diff) / sizeof(primes_diff[0]) - 8 + 1)

static constexpr unsigned char primes_diff8[] = {
#define P(a,b,c,d) b,
  PRIMES_H_CONTENTS
#undef P
  0,0,0,0,0,0,0                           /* 7 sentinels for 8-way loop */
};

struct primes_dtab {
  uintmax_t binv;
  uintmax_t lim;
};

static constexpr struct primes_dtab primes_dtab[] = {
#define P(a,b,c,d) {c,d},
  PRIMES_H_CONTENTS
#undef P
  {1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0} /* 7 sentinels for 8-way loop */
};

static constexpr void
factor_insert_refind(struct factors *factors, uintmax_t p, int i, int off) {
  for (int j = 0; j < off; ++j)
    p += primes_diff[i + j];
  factor_insert_multiplicity(factors, p, 1);
}

/* Trial division with odd primes uses the following trick.

   Let p be an odd prime, and B = 2^{64}.  For simplicity,
   consider the case t < B (this is the second loop below).

   From our tables we get

     binv = p^{-1} (mod B)
     lim = floor ((B-1) / p).

   First assume that t is a multiple of p, t = q * p.  Then 0 <= q <= lim
   (and all quotients in this range occur for some t).

   Then t = q * p is true also (mod B), and p is invertible we get

     q = t * binv (mod B).

   Next, assume that t is *not* divisible by p.  Since multiplication
   by binv (mod B) is a one-to-one mapping,

     t * binv (mod B) > lim,

   because all the smaller values are already taken.

   This can be summed up by saying that the function

     q(t) = binv * t (mod B)

   is a permutation of the range 0 <= t < B, with the curious property
   that it maps the multiples of p onto the range 0 <= q <= lim, in
   order, and the non-multiples of p onto the range lim < q < B.
 */

static constexpr uintmax_t
factor_using_division(uintmax_t t0, struct factors *factors) {
  if (t0 % 2 == 0) {
    int cnt = __builtin_ctzll(t0);
    t0 >>= cnt;
    factor_insert_multiplicity(factors, 2, cnt);
  }

#define DIVBLOCK(I) do { \
    while (true) { \
      uintmax_t q = t0 * pd[I].binv;                                \
      if ((q > pd[I].lim)) [[likely]]                               \
        break;                                                      \
      t0 = q;                                                       \
      factor_insert_refind(factors, p, i + 1, I);                   \
    }                                                               \
  } while (0)

  uintmax_t p = 3;
  for (size_t i = 0; i < PRIMES_PTAB_ENTRIES; i += 8) {
    const struct primes_dtab *pd = &primes_dtab[i];
    DIVBLOCK(0);
    DIVBLOCK(1);
    DIVBLOCK(2);
    DIVBLOCK(3);
    DIVBLOCK(4);
    DIVBLOCK(5);
    DIVBLOCK(6);
    DIVBLOCK(7);
    p += primes_diff8[i];
    if (p * p > t0)
      break;
  }
  return t0;
}

/* Entry i contains (2i+1)^(-1) mod 2^8.  */
static constexpr unsigned char  binvert_table[128] = {
  0x01, 0xAB, 0xCD, 0xB7, 0x39, 0xA3, 0xC5, 0xEF,
  0xF1, 0x1B, 0x3D, 0xA7, 0x29, 0x13, 0x35, 0xDF,
  0xE1, 0x8B, 0xAD, 0x97, 0x19, 0x83, 0xA5, 0xCF,
  0xD1, 0xFB, 0x1D, 0x87, 0x09, 0xF3, 0x15, 0xBF,
  0xC1, 0x6B, 0x8D, 0x77, 0xF9, 0x63, 0x85, 0xAF,
  0xB1, 0xDB, 0xFD, 0x67, 0xE9, 0xD3, 0xF5, 0x9F,
  0xA1, 0x4B, 0x6D, 0x57, 0xD9, 0x43, 0x65, 0x8F,
  0x91, 0xBB, 0xDD, 0x47, 0xC9, 0xB3, 0xD5, 0x7F,
  0x81, 0x2B, 0x4D, 0x37, 0xB9, 0x23, 0x45, 0x6F,
  0x71, 0x9B, 0xBD, 0x27, 0xA9, 0x93, 0xB5, 0x5F,
  0x61, 0x0B, 0x2D, 0x17, 0x99, 0x03, 0x25, 0x4F,
  0x51, 0x7B, 0x9D, 0x07, 0x89, 0x73, 0x95, 0x3F,
  0x41, 0xEB, 0x0D, 0xF7, 0x79, 0xE3, 0x05, 0x2F,
  0x31, 0x5B, 0x7D, 0xE7, 0x69, 0x53, 0x75, 0x1F,
  0x21, 0xCB, 0xED, 0xD7, 0x59, 0xC3, 0xE5, 0x0F,
  0x11, 0x3B, 0x5D, 0xC7, 0x49, 0x33, 0x55, 0xFF
};

/* Compute n^(-1) mod B, using a Newton iteration.  */
#define binv(inv,n) do { \
    uintmax_t  __n = (n); \
    uintmax_t __inv = binvert_table[(__n / 2) & 0x7F]; \
    __inv = 2 * __inv - __inv * __inv * __n;     \
    __inv = 2 * __inv - __inv * __inv * __n;     \
    __inv = 2 * __inv - __inv * __inv * __n;     \
    (inv) = __inv;                               \
  } while (0)

/* Modular two-word multiplication, r = a * b mod m, with mi = m^(-1) mod B.
   Both a and b must be in redc form, the result will be in redc form too.  */
[[gnu::const]] constexpr uintmax_t
mulredc(uintmax_t a, uintmax_t b, uintmax_t m, uintmax_t mi)
{
  uint128 product = uint128(a) * uint128(b);
  uintmax_t rh = uintmax_t(product >> 64);
  uintmax_t rl = uintmax_t(product);
  uintmax_t q = rl * mi;
  product = uint128(q) * uint128(m);
  uintmax_t th = uintmax_t(product >> 64);
  if (rh < th) {
    return (rh - th) + m;
  } else {
    return (rh - th);
  }
}

[[gnu::const]] constexpr uintmax_t
powm(uintmax_t b, uintmax_t e, uintmax_t n, uintmax_t ni, uintmax_t one) {
  uintmax_t y = one;
  if (e & 1)
    y = b;
  while (e != 0) {
    b = mulredc(b, b, n, ni);
    e >>= 1;
    if (e & 1)
      y = mulredc(y, b, n, ni);
  }
  return y;
}

[[gnu::const]] constexpr bool
millerrabin(uintmax_t n, uintmax_t ni, uintmax_t b, uintmax_t q, int k, uintmax_t one) {
  uintmax_t y = powm(b, q, n, ni, one);
  uintmax_t nm1 = n - one;      /* -1, but in redc representation.  */
  if (y == one || y == nm1)
    return true;
  for (int i = 1; i < k; ++i) {
    y = mulredc(y, y, n, ni);
    if (y == nm1)
      return true;
    if (y == one)
      return false;
  }
  return false;
}

/* Lucas' prime test.  The number of iterations vary greatly, up to a few dozen
   have been observed.  The average seem to be about 2.  */
[[gnu::const]] constexpr bool prime_p(uintmax_t n) {
  uintmax_t ni;
  struct factors factors;

  if (n <= 1)
    return false;

  /* We have already cast out small primes.  */
  if (n < FIRST_OMITTED_PRIME * FIRST_OMITTED_PRIME)
    return true;

  /* Precomputation for Miller-Rabin.  */
  uintmax_t q = n - 1;
  int k = __builtin_ctzll(q);
  q >>= k;

  uintmax_t a = 2;
  binv(ni, n);                 /* ni <- 1/n mod B */
  uintmax_t one = uintmax_t((uint128(1) << 64) % n);
  uintmax_t a_prim = uintmax_t((uint128(one) + one) % n);

  /* Perform a Miller-Rabin test, finds most composites quickly.  */
  if (!millerrabin(n, ni, a_prim, q, k, one))
    return false;

  /* Factor n-1 for Lucas.  */
  factor(n-1, &factors);

  /* Loop until Lucas proves our number prime, or Miller-Rabin proves our
     number composite.  */
  for (size_t r = 0; r < PRIMES_PTAB_ENTRIES; ++r) {
    bool is_prime = true;

    for (int i = 0; i < factors.nfactors && is_prime; ++i) {
      is_prime = powm(a_prim, (n - 1) / factors.p[i], n, ni, one) != one;
    }
    if (is_prime)
      return true;

    a += primes_diff[r];      /* Establish new base.  */

    a_prim = uintmax_t((uint128(one) * a) % n);
    if (!millerrabin(n, ni, a_prim, q, k, one))
      return false;

    // Miller-Rabin for a=2,3,5,7,11,13,17,19,23,29,31,37 is enough for n < 2^64
    if (r == 10) return true;
  }

  assert(!"Lucas prime test failure.  This should not happen");
}

constexpr void
factor_using_pollard_rho(uintmax_t n, unsigned long int a, struct factors *factors) {
  uintmax_t t, ni, g;

  unsigned long int k = 1;
  unsigned long int l = 1;

  uintmax_t P = uintmax_t((uint128(1) << 64) % n);
  uintmax_t x = uintmax_t((uint128(P) + P) % n);
  uintmax_t y = x;
  uintmax_t z = x;

  while (n != 1) {
      assert(a < n);
      binv(ni, n);             /* FIXME: when could we use old 'ni' value?  */
      [&]() {
        while (true) {
          do {
            x = mulredc(x, x, n, ni);
            x = uintmax_t((uint128(x) + a) % n);
            t = uintmax_t((uint128(z) - x) % n);
            P = mulredc(P, t, n, ni);
            if (k % 32 == 1) {
              if (gcd_odd(P, n) != 1)
                return; // goto factor_found;
              y = x;
            }
          } while (--k != 0);

          z = x;
          k = l;
          l = 2 * l;
          for (unsigned long int i = 0; i < k; i++) {
            x = mulredc(x, x, n, ni);
            x = uintmax_t((uint128(x) + a) % n);
          }
          y = x;
        }
      }();

    // factor_found:
      do {
        y = mulredc(y, y, n, ni);
        y = uintmax_t((uint128(y) + a) % n);
        t = uintmax_t((uint128(z) - y) % n);
        g = gcd_odd(t, n);
      } while (g == 1);

      if (n == g) {
        /* Found n itself as factor.  Restart with different params.  */
        factor_using_pollard_rho(n, a + 1, factors);
        return;
      }

      n /= g;

      if (prime_p(g)) {
        factor_insert_multiplicity(factors, g, 1);
      } else {
        factor_using_pollard_rho(g, a + 1, factors);
      }

      if (prime_p(n)) {
        factor_insert_multiplicity(factors, n, 1);
        break;
      }

      x %= n;
      z %= n;
      y %= n;
    }
}

/* Compute the prime factors of the 128-bit number (T1,T0), and put the
   results in FACTORS.  */
constexpr void factor(uintmax_t t0, struct factors *factors) {
  factors->nfactors = 0;
  factors->plarge[1] = 0;
  if (t0 < 2)
    return;
  t0 = factor_using_division(t0, factors);
  if (t0 < 2)
    return;
  if (prime_p(t0))
    factor_insert_multiplicity(factors, t0, 1);
  else
    factor_using_pollard_rho(t0, 1, factors);
}

constexpr
std::vector<uintmax_t> factors_of(uintmax_t t0) {
    struct factors factors;
    factor(t0, &factors);
    std::vector<uintmax_t> result;
    for (int j = 0; j < factors.nfactors; ++j) {
        for (int k = 0; k < factors.e[j]; ++k) {
            result.push_back(factors.p[j]);
        }
    }
    return result;
}

using V = std::vector<uintmax_t>;

// https://oeis.org/A014233
static_assert((factors_of(2047) == V{23, 89}));
static_assert((factors_of(1373653) == V{829, 1657}));
static_assert((factors_of(25326001) == V{2251, 11251}));
static_assert((factors_of(3215031751) == V{151, 751, 28351}));
static_assert((factors_of(2152302898747) == V{6763, 10627, 29947}));
static_assert((factors_of(3474749660383) == V{1303, 16927, 157543}));
static_assert((factors_of(341550071728321) == V{10670053, 32010157}));
static_assert((factors_of(3825123056546413051) == V{149491, 747451, 34233211}));

// Miscellaneous test cases
static_assert((factors_of(100) == V{2, 2, 5, 5}));
static_assert((factors_of(2147462143) == V{2147462143}));
static_assert((factors_of(2147462142) == V{2, 3, 7, 7, 757, 9649}));
static_assert((factors_of(15032235001) == V{7, 2147462143}));
static_assert((factors_of(214746214200) == V{2, 2, 2, 3, 5, 5, 7, 7, 757, 9649}));
static_assert((factors_of(18361375334787046697uLL) == V{18361375334787046697uLL}));
//static_assert((factors_of(4611593655618152449) == V{2147462143, 2147462143}));
//static_assert((factors_of(14862346268936648689uLL) == V{3855171367, 3855171367}));
