.section ".rodata"
.global vivi_initialize
  .type vivi_initialize,@object
  .size vivi_initialize, .-vivi_initialize
vivi_initialize:
.incbin "vivi_initialize.as"
.byte 0
