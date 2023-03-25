@cipher Keccak_p400

uint10[5][5] rho_val = {{0, 36, 3, 105, 210},
                        {1, 300, 10, 45, 66},
                        {190, 6, 171, 15, 253},
                        {28, 55, 153, 21, 120},
                        {91, 276, 231, 136, 78}};

uint16[24] rc = {1,32898,32906,32768,32907,1,30897,32777,138,136,32777,10,32907,139,32905,32771,32770,128,32778,10,32897,32896,1,32776};

r_fn uint1[400] round_function1(uint8 r, uint1[32] key, uint1[400] input) {
    uint1[400] parity1;
    uint1[400] parity2;
    for (i from 0 to 4) {
        for (j from 0 to 4) {
            for (k from 0 to 15) {
                parity1[(i*5+j)*16+k] = input[(((i-1)%5)*5+0)*16+k] ^ input[(((i-1)%5)*5+1)*16+k] ^ input[(((i-1)%5)*5+2)*16+k] ^ input[(((i-1)%5)*5+3)*16+k] ^ input[(((i-1)%5)*5+4)*16+k];
                parity2[(i*5+j)*16+k] = input[(((i+1)%5)*5+0)*16+(k-1)%16] ^ input[(((i+1)%5)*5+1)*16+(k-1)%16] ^ input[(((i+1)%5)*5+2)*16+(k-1)%16] ^ input[(((i+1)%5)*5+3)*16+(k-1)%16] ^ input[(((i+1)%5)*5+4)*16+(k-1)%16];
            }
        }
    }
    uint1[400] theta_out;
    for (i from 0 to 4) {
        for (j from 0 to 4) {
            for (k from 0 to 15) {
                theta_out[(i*5+j)*16+k] = input[(i*5+j)*16+k] ^ parity1[(i*5+j)*16+k] ^ parity2[(i*5+j)*16+k];
            }
        }
    }

    uint1[400] rho_out;
    for (i from 0 to 4) {
        for (j from 0 to 4) {
            for (k from 0 to 15) {
                rho_out[(i*5+j)*16+(k+rho_val[i][j])%16] = theta_out[(i*5+j)*16+k];
            }
        }
    }

    uint1[400] pi_out;
    for (i from 0 to 4) {
        for (j from 0 to 4) {
            for (k from 0 to 15) {
                pi_out[(i*5+j)*16+k] = rho_out[(((i+3*j)%5)*5+i)*16+k];
            }
        }
    }

    uint1[400] chi_out;
    for (i from 0 to 4) {
        for (j from 0 to 4) {
            for (k from 0 to 15) {
                chi_out[(i*5+j)*16+k] = pi_out[(i*5+j)*16+k] ^ pi_out[(((i+1)%5)*5+j)*16+k] ^ 1 & pi_out[(((i+2)%5)*5+j)*16+k];
            }
        }
    }

    for (i from 0 to 15) {
        chi_out[i] = chi_out[i] ^ rc[r-1][16-i];
    }
    return chi_out;
}

fn uint1[400] enc(uint1[32] key, uint1[400] r_plaintext){
    for (i from 1 to 24) {
        r_plaintext = round_function1(i, key, r_plaintext);
    }
    return r_plaintext;
}