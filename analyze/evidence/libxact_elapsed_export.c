float __fastcall CLibXact::GetSoundLabelLapse(CLibXact *this, char *a2)
{
  unsigned __int16 *v2; // rax

  if ( *((_DWORD *)this + 145415) == 2 && a2 != nullptr && (v2 = (unsigned __int16 *)sub_180005E90(Src: a2)) != nullptr ) /*0x18000724d*/
    return sub_180004040(a1: (char *)&unk_188F8BA30 + 50592 * *v2, a2: v2[1]); /*0x18000726b*/
  else
    return -1.0; /*0x180007270*/
}
