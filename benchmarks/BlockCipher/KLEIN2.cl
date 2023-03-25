@cipher KLEIN

sbox uint4[16] s = {7, 4, 10, 9, 1, 15, 11, 0, 12, 3, 2, 6, 8, 14, 13, 5};
pbox uint[64] p = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
pboxm uint8[4][4] M = {{2, 3, 1, 1},
                        {1, 2, 3, 1},
                        {1, 1, 2, 3},
                        {3, 1, 1, 2}};
ffm uint8[16][16] M = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                      {0, 2, 4, 6, 8, 10, 12, 14, 16, 1, 3, 5, 7, 9, 11, 13, 15},
                      {0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 16, 2, 5, 8, 11, 14},
                      {0, 4, 8, 12, 16, 3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13},
                      {0, 5, 10, 15, 3, 8, 13, 1, 6, 11, 16, 4, 9, 14, 2, 7, 12},
                      {0, 6, 12, 1, 7, 13, 2, 8, 14, 3, 9, 15, 4, 10, 16, 5, 11},
                      {0, 7, 14, 4, 11, 1, 8, 15, 5, 12, 2, 9, 16, 6, 13, 3, 10},
                      {0, 8, 16, 7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9},
                      {0, 9, 1, 10, 2, 11, 3, 12, 4, 13, 5, 14, 6, 15, 7, 16, 8},
                      {0, 10, 3, 13, 6, 16, 9, 2, 12, 5, 15, 8, 1, 11, 4, 14, 7},
                      {0, 11, 5, 16, 10, 4, 15, 9, 3, 14, 8, 2, 13, 7, 1, 12, 6},
                      {0, 12, 7, 2, 14, 9, 4, 16, 11, 6, 1, 13, 8, 3, 15, 10, 5},
                      {0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3, 16, 12, 8, 4},
                      {0, 14, 11, 8, 5, 2, 16, 13, 10, 7, 4, 1, 15, 12, 9, 6, 3},
                      {0, 15, 13, 11, 9, 7, 5, 3, 1, 16, 14, 12, 10, 8, 6, 4, 2},
                      {0, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}};

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
    uint8[4] m_in1;
    uint8[4] m_in2;
    for (i from 0 to 3) {
        m_in1[i] = touint(p_out[i*4], p_out[i*4+1], p_out[i*4+2], p_out[i*4+3],
                   p_out[i*4+4], p_out[i*4+5], p_out[i*4+6], p_out[i*4+7]);
    }
    for (i from 4 to 7) {
        m_in2[i] = touint(p_out[i*4], p_out[i*4+1], p_out[i*4+2], p_out[i*4+3],
                   p_out[i*4+4], p_out[i*4+5], p_out[i*4+6], p_out[i*4+7]);
    }
    uint8[4] m_out1 = M * m_in1;
    uint8[4] m_out2 = M * m_in2;
    uint1[64] rtn;
    for (i from 0 to 3) {
        for (j from 0 to 7) {
            rtn[i*8+j] = m_out1[i][j];
            rtn[i*8+j+32] = m_out2[i][j];
        }
    }

	return rtn;
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