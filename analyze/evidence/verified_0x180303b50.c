void *sub_180303B50()
{
  if ( (dword_184AE4B44 & 1) == 0 ) /*0x180303b5e*/
  {
    dword_184AE4B44 |= 1u; /*0x180303b6d*/
    sub_1803015E0(a1: &unk_184AE4B50); /*0x180303b73*/
    atexit(a1: nullsub_105); /*0x180303b7f*/
  }
  return &unk_184AE4B50; /*0x180303b87*/
}
