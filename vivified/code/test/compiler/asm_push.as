asm {
  push true, false, null, undefined
  push 1, 1.2, 1f, 1d
  push "stacked", pool 0, pool 1234
  push reg 0
}
