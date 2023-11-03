void platform_sleep(int n) {
  n *= 6;
  while(n > 0) {
    __asm__("bit $C082");
    __asm__("lda #$ff");
    __asm__("jsr $fca8"); /* MONWAIT */
    __asm__("bit $C080");
    n--;
  }
}
