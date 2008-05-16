// equal precedence
trace (1 == 2 != 3 === 4 !== 5);
trace (1 !== 2 === 3 != 4 == 5);

trace (1 < 2 > 3 <= 4 >= 5 instanceof a);
trace (1 instanceof a >= 3 <= 4 > 5 < 6);

trace (1 << 2 >> 3 >>> 4);
trace (1 >>> 2 >> 3 << 4);

trace (1 + 2 - 3);
trace (1 - 2 + 3);

trace (1 * 2 / 3 % 4);
trace (1 % 2 / 3 * 4);

// different precedence
trace (1 && 2 || 3);
trace (1 || 2 && 3);

trace (1 | 2 && 3);
trace (1 && 2 | 3);

trace (1 ^ 2 | 3);
trace (1 | 2 ^ 3);

trace (1 & 2 ^ 3);
trace (1 ^ 2 & 3);

trace (1 == 2 & 3);
trace (1 & 2 == 3);

trace (1 < 2 == 3);
trace (1 == 2 < 3);

trace (1 << 2 < 3);
trace (1 < 2 << 3);

trace (1 + 2 << 3);
trace (1 << 2 + 3);

trace (1 * 2 + 3);
trace (1 + 2 * 3);
