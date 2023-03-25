# 128-bit keyed permutation for TinyJAMBU, called P_n
@cipher TinyJAMBU_Keyed_Permutation

pbox uint[128] p = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,0};

r_fn uint1[128] round_function(uint8 r, uint1[128] key, uint1[128] input) {
    uint1 t = input[70] & input[85];
    uint8 i = r % 128;
    uint1 feedback = input[0] ^ input[47] ^ ~t ^ input[91] ^ key[i];
    uint1[128] p_out = p<input>;
    p_out[127] = feedback;
	return p_out;
}

fn uint1[64] enc(uint1[128] key, uint1[128] r_plaintext){
    for (i from 1 to 1024) {
        r_plaintext = round_function(i, key, r_plaintext);
    }
    return r_plaintext;
}