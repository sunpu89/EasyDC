# can not differential
@cipher Xoodoo_permutation

uint128[12] rc = {18,416,240,896,44,96,20,288,224,960,56,88};

r_fn uint1[384] round_function(uint8 r, uint1[32] key, uint1[384] input) {
    uint1[384] xor_out;
    uint1[384] p1 = input <<< 37;
    uint1[384] p2 = input <<< 46;
    xor_out = p1 ^ p2;

    uint1[128] a0 = View(input, 0, 127);
    uint1[128] a1 = View(input, 128, 255);
    uint1[128] a2 = View(input, 256, 383);
    a0 = a0 ^ View(xor_out, 0, 127);
    a1 = a1 ^ View(xor_out, 128, 255);
    a2 = a2 ^ View(xor_out, 256, 383);

    a1 = a1 <<< 32;
    a2 = a2 <<< 11;
    a0 = a0 ^ rc[r];

    uint1[128] b0 = ~a1 & a2;
    uint1[128] b1 = ~a2 & a0;
    uint1[128] b2 = ~a0 & a1;
    a0 = a0 ^ b0;
    a1 = a1 ^ b1;
    a2 = a2 ^ b2;
    a1 = a1 <<< 1;
    a2 = a2 <<< 40;

    uint1[384] rtn;
    for (i from 0 to 127) {
        rtn[i] = a0[i];
        rtn[i + 128] = a1[i];
        rtn[i + 256] = a2[i];
    }
    return rtn;
}

fn uint1[384] enc(uint1[32] key, uint1[384] r_plaintext){
    for (i from 1 to 12) {
        r_plaintext = round_function(i, key, r_plaintext);
    }
    return r_palintext;
}