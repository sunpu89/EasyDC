# the implementation without "touint" function,
# and the XOR operation subjects are uint1 arrays other than bits
@cipher KLEIN

sbox uint4[16] s = {7, 4, 10, 9, 1, 15, 11, 0, 12, 3, 2, 6, 8, 14, 13, 5};
pbox uint[64] p = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
pbox uint[64] x1 = {24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55};
pbox uint[64] x2 = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};
pbox uint[64] x3 = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 32, 33, 34, 35, 36, 37, 38, 39};
pbox uint[64] x4 = {1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31, 24, 33, 34, 35, 36, 37, 38, 39, 32, 41, 42, 43, 44, 45, 46, 47, 40, 49, 50, 51, 52, 53, 54, 55, 48, 57, 58, 59, 60, 61, 62, 63, 56};
pbox uint[64] x5 = {9, 10, 11, 12, 13, 14, 15, 8, 17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31, 24, 1, 2, 3, 4, 5, 6, 7, 0, 41, 42, 43, 44, 45, 46, 47, 40, 49, 50, 51, 52, 53, 54, 55, 48, 57, 58, 59, 60, 61, 62, 63, 56, 33, 34, 35, 36, 37, 38, 39, 32};

r_fn uint1[64] round_function1(uint8 r, uint1[64] key, uint1[64] input) {
	uint1[64] n_input = input ^ key;
    uint1[64] s_out;
    for (i from 0 to 15) {
        uint1[4] sbox_in = View(n_input, i*4, i*4+3);
        uint1[4] sbox_out = s<sbox_in>;
        s_out[i * 4 + 0] = sbox_out[0];
        s_out[i * 4 + 1] = sbox_out[1];
        s_out[i * 4 + 2] = sbox_out[2];
        s_out[i * 4 + 3] = sbox_out[3];
    }
    uint1[64] p_out = p<s_out>;
    uint1[64] x_in1 = x1<p_out>;
    uint1[64] x_in2 = x2<p_out>;
    uint1[64] x_in3 = x3<p_out>;
    uint1[64] x_in4 = x4<p_out>;
    uint1[64] x_in5 = x5<p_out>;
    uint1[64] x_out;
    for (i from 0 to 63) {
        x_out[i] = x_in1[i] ^ x_in2[i] ^ x_in3[i] ^ x_in4[i] ^ x_in5[i];
    }
	return x_out;
}

r_fn uint1[64] round_function2(uint8 r, uint1[64] key, uint1[64] input) {
    input = input ^ key;
    return input;
}

fn uint1[64] enc(uint1[832] key, uint1[64] r_plaintext){
    for (i from 1 to 12) {
        r_plaintext = round_function1(i, View(key, (i - 1) * 64, i * 64 - 1), r_plaintext);
    }
    r_plaintext = round_function2(13, View(key, 768, 831), r_plaintext);
    return r_plaintext;
}