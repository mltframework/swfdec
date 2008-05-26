asm {
  try has_catch has_finally reserved1 reserved2 reserved3 reserved4 reserved5 1 catch_start finally_start end
  try "a" catch_start finally_start end
catch_start:
  try has_catch "a" finally_start end end
finally_start:
  try "a" end end end
  try has_catch has_finally 1 end end end
end:
}
