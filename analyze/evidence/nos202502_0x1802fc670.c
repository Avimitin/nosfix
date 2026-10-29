__int64 __fastcall sub_1802FC670(const char *a1)
{
  __int64 result; // rax
  double v3; // xmm6_8
  int v4; // ecx
  int v5; // esi
  const char *v6; // rbx
  CLibXact *Instance; // rax
  DWORD SoundLabelLapse; // edx
  CLibXact *v9; // rax

  result = *(unsigned int *)a1; /*0x1802fc676*/
  v3 = *(double *)&qword_18070B2F8; /*0x1802fc680*/
  if ( (int)result > 0 ) /*0x1802fc68a*/
  {
    v4 = *((_DWORD *)a1 + 80); /*0x1802fc69f*/
    if ( v4 < 1 ) /*0x1802fc6a8*/
    {
      v5 = (int)((double)(int)result * 1000.0 / *(double *)&qword_18070B2F8); /*0x1802fc6e8*/
      v6 = (const char *)(sub_180360060() + 680); /*0x1802fc6f2*/
      Instance = (CLibXact *)CLibXact::getInstance(); /*0x1802fc6f9*/
      if ( (int)CLibXact::GetSoundLabelLapse(this: Instance, a2: v6) > 0 ) /*0x1802fc711*/
      {
        sub_180360060(); /*0x1802fc723*/
        v9 = (CLibXact *)CLibXact::getInstance(); /*0x1802fc728*/
        SoundLabelLapse = (int)CLibXact::GetSoundLabelLapse(this: v9, a2: a1 + 8); /*0x1802fc73b*/
      }
      else
      {
        SoundLabelLapse = timeGetTime() - *((_DWORD *)a1 + 67); /*0x1802fc71b*/
      }
      result = *((unsigned int *)a1 + 76); /*0x1802fc73f*/
      if ( (int)result <= 0 ) /*0x1802fc74c*/
      {
        if ( (int)result < 0 ) /*0x1802fc76b*/
        {
          result = (unsigned int)(int)((double)(int)result * -1.0 * 1000.0 / v3); /*0x1802fc789*/
          if ( SoundLabelLapse >= (unsigned int)result ) /*0x1802fc790*/
            SoundLabelLapse -= result; /*0x1802fc796*/
          else
            SoundLabelLapse = 0; /*0x1802fc792*/
        }
      }
      else
      {
        result = (unsigned int)(int)((double)(int)result * 1000.0 / v3); /*0x1802fc762*/
        SoundLabelLapse += result; /*0x1802fc767*/
      }
      *((_DWORD *)a1 + 66) = SoundLabelLapse - v5; /*0x1802fc79a*/
    }
    else
    {
      result = (unsigned int)(v4 - 1); /*0x1802fc6aa*/
      *((_DWORD *)a1 + 80) = result; /*0x1802fc6b3*/
      *((_DWORD *)a1 + 66) = 16 * v4 + 10; /*0x1802fc6b9*/
    }
  }
  else
  {
    *((_DWORD *)a1 + 66) = 0; /*0x1802fc68e*/
  }
  return result; /*0x1802fc694*/
}
