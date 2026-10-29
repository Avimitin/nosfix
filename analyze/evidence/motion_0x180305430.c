float *__fastcall sub_180305430(
        int a1,
        __int64 a2,
        int *a3,
        int *a4,
        float *a5,
        float *a6,
        float *a7,
        float *a8,
        int a9,
        int a10,
        _DWORD *a11,
        _DWORD *a12)
{
  float v14; // xmm9_4
  int v15; // edx
  __int64 v16; // r8
  float v17; // xmm2_4
  int v18; // ecx
  _DWORD *v19; // r9
  int v20; // r9d
  int v21; // r8d
  int v22; // r10d
  float v23; // xmm7_4
  int v24; // edx
  __int64 v25; // rcx
  int v26; // ecx
  __int64 v27; // rax
  int v28; // ecx
  float *result; // rax
  int v30; // edx
  int v31; // ecx
  int v32; // ecx
  float v33; // xmm9_4
  int v34; // edx
  int v35; // ecx
  int v36; // ecx
  int v37; // edx
  __int64 v38; // rcx
  float v39; // xmm0_4
  __int64 v40; // rax
  int v41; // ecx
  float v42; // xmm8_4

  v14 = 0.0; /*0x18030547e*/
  v15 = *(_DWORD *)(sub_18018B090(a1: (unsigned int)dword_1813668B0[a1 + 128973]) + 60); /*0x180305482*/
  *a3 = v18; /*0x180305485*/
  *v19 = 720 - v15; /*0x180305498*/
  if ( a9 != -1 && a10 != -1 ) /*0x1803054ab*/
  {
    v20 = *(_DWORD *)(v16 + 4LL * a10 + 515776); /*0x1803054ad*/
    v21 = *(_DWORD *)(v16 + 4LL * a9 + 515776); /*0x1803054b8*/
    v22 = dword_1813E476C; /*0x1803054c3*/
    *a3 = v21 + (dword_1813E476C + v20 - v21 + 1) / 2; /*0x1803054d7*/
    if ( a11 != nullptr ) /*0x1803054e4*/
      *a11 = v21; /*0x1803054e6*/
    if ( a12 != nullptr ) /*0x1803054f4*/
      *a12 = v20 + v22 + 1; /*0x1803054fd*/
  }
  if ( (float)(*(float *)&dword_1813668D4 - v17) >= 0.0 ) /*0x180305512*/
    v23 = (float)(*(float *)&dword_1813668D4 - v17) / *(float *)&dword_1813668D4; /*0x180305519*/
  else
    v23 = 0.0; /*0x180305514*/
  if ( (_DWORD)qword_1813E4724 == 0 ) /*0x18030553b*/
  {
    v23 = v23 * v23; /*0x180305554*/
    v24 = 920 - *(_DWORD *)(sub_18018B090(a1: 720) + 60); /*0x180305558*/
    if ( v23 >= 0.5 ) /*0x18030555f*/
    {
      if ( v23 == 1.0 ) /*0x1803055bd*/
      {
        v27 = sub_18018B090(a1: v25); /*0x1803055bf*/
        v26 = v28 - *(_DWORD *)(v27 + 60); /*0x1803055c4*/
      }
      else
      {
        v26 = (int)(float)((float)(v24 - 250) /*0x180305617*/
                         - (float)(cosf(
                                     X: fmaxf(
                                       (float)((float)((float)(int)(float)((float)v24 * v23) - (float)((float)v24 * 0.5))
                                             / (float)((float)v24 * 0.5))
                                     * 1.5707964,
                                       0.0))
                                 * (float)(v24 - 250)))
            + 50;
      }
    }
    else
    {
      v26 = (int)(float)(300.0 /*0x1803055b1*/
                       - (float)(sinf(
                                   X: fmaxf(
                                     fminf(
                                       1.5707964,
                                       (float)((float)(int)(float)((float)v24 * v23) / (float)((float)v24 * 0.5))
                                     * 1.5707964),
                                     0.0))
                               * 250.0));
    }
    *a4 = v26; /*0x18030561d*/
    v14 = cosf(X: fmaxf(v23 * 1.5707964, 0.0)) * (float)(1.0 - *(float *)&dword_1813E4970); /*0x180305647*/
    *a3 = (int)(float)((float)((float)(640 - *a3) * v14) + (float)*a3); /*0x180305667*/
    goto LABEL_48; /*0x180305669*/
  }
  if ( (_DWORD)qword_1813E4724 != 3 ) /*0x180305675*/
  {
    switch ( (_DWORD)qword_1813E4724 ) /*0x180305723*/
    {
      case 1: /*0x180305723*/
        v23 = v23 * v23; /*0x180305725*/
        v30 = *(_DWORD *)(sub_18018B090(a1: 720) + 60); /*0x180305732*/
        if ( v23 == 1.0 ) /*0x180305737*/
          v32 = v31 - v30; /*0x180305739*/
        else
          v32 = (int)(float)((float)((float)(740 - v30) * v23) - 20.0); /*0x180305757*/
        *a4 = v32; /*0x18030575f*/
        v33 = 1.0 - v23; /*0x180305761*/
        goto LABEL_32; /*0x180305761*/
      case 4: /*0x180305723*/
        v23 = v23 * v23; /*0x1803057af*/
        v34 = *(_DWORD *)(sub_18018B090(a1: 720) + 60); /*0x1803057bc*/
        if ( v23 == 1.0 ) /*0x1803057c1*/
          v36 = v35 - v34; /*0x1803057c3*/
        else
          v36 = (int)(float)((float)((float)(740 - v34) * v23) - 20.0); /*0x1803057e1*/
        *a4 = v36; /*0x1803057e5*/
        v14 = ((float)(1.0 - *(float *)&dword_1813E4970) - 0.2) * (float)(1.0 - v23); /*0x180305824*/
        *a3 = (int)(float)((float)((float)(640 - *a3) * v14) + (float)*a3); /*0x180305836*/
        break;
      case 2: /*0x180305723*/
        v23 = v23 * v23; /*0x180305846*/
        *a4 = (int)(float)((float)((float)(770 - *(_DWORD *)(sub_18018B090(a1: 720) + 60)) * v23) - 50.0); /*0x180305879*/
        v33 = cosf(X: fmaxf(v23 * 1.5707964, 0.0)); /*0x180305885*/
LABEL_32:
        v14 = v33 * (float)(1.0 - *(float *)&dword_1813E4970); /*0x180305766*/
        *a3 = (int)(float)((float)((float)(640 - *a3) * v14) + (float)*a3); /*0x18030579b*/
        break; /*0x18030579d*/
      case 5: /*0x180305723*/
        v37 = 920 - *(_DWORD *)(sub_18018B090(a1: 720) + 60); /*0x1803058a5*/
        if ( v23 >= 0.5 ) /*0x1803058ac*/
        {
          if ( v23 == 1.0 ) /*0x180305939*/
          {
            v40 = sub_18018B090(a1: v38); /*0x18030593b*/
            *a4 = v41 - *(_DWORD *)(v40 + 60); /*0x180305943*/
          }
          else
          {
            *a4 = (int)(float)((float)(v37 - 250) /*0x18030599b*/
                             - (float)(cosf(
                                         X: fmaxf(
                                           (float)((float)((float)(int)(float)((float)v37 * v23)
                                                         - (float)((float)v37 * 0.5))
                                                 / (float)((float)v37 * 0.5))
                                         * 1.5707964,
                                           0.0))
                                     * (float)(v37 - 250)))
                + 50;
          }
        }
        else
        {
          v39 = sinf( /*0x1803058ec*/
                  X: fmaxf(
                    fminf(
                      1.5707964,
                      (float)((float)(int)(float)((float)v37 * v23) / (float)((float)v37 * 0.5)) * 1.5707964),
                    0.0));
          *a4 = (int)(float)(300.0 - (float)(v39 * 250.0)); /*0x180305915*/
          v14 = fmaxf(0.0, cosf(X: fmaxf(v39 * 1.5707964, 0.0)) * (float)(1.0 - *(float *)&dword_1813E4970)); /*0x18030592c*/
        }
        if ( v23 <= 0.5 ) /*0x1803059a1*/
          v14 = 0.0; /*0x1803059a3*/
        break;
      default:
        break; /*0x180305723*/
    }
LABEL_48:
    v42 = 1.0 - v14; /*0x1803059a7*/
    if ( a6 != nullptr ) /*0x1803059b7*/
      *a6 = v42 + 0.0; /*0x1803059c2*/
    if ( a5 != nullptr ) /*0x1803059d1*/
      *a5 = v42; /*0x1803059d3*/
    if ( (_DWORD)qword_1813E4724 == 1 || (_DWORD)qword_1813E4724 == 4 ) /*0x1803059e6*/
      v23 = v23 * 3.0; /*0x1803059e8*/
    if ( a7 != nullptr ) /*0x1803059fb*/
    {
      if ( v23 >= 0.5 ) /*0x180305a01*/
        *a7 = 1.0; /*0x180305a15*/
      else
        *a7 = (float)(v23 + v23) + 0.0; /*0x180305a0f*/
    }
    result = a8; /*0x180305a1b*/
    if ( a8 != nullptr ) /*0x180305a26*/
    {
      if ( v23 < 0.5 ) /*0x180305a30*/
      {
        *a8 = (float)(v23 + v23) + 0.0; /*0x180305a3f*/
        return result; /*0x180305a3f*/
      }
      goto LABEL_25; /*0x180305a30*/
    }
    return result; /*0x180305a26*/
  }
  *a4 = (int)(float)((float)((float)(770 - *(_DWORD *)(sub_18018B090(a1: 720) + 60)) * v23) - 50.0); /*0x18030569f*/
  if ( a5 != nullptr ) /*0x1803056ac*/
    *a5 = 1.0; /*0x1803056ae*/
  if ( a6 != nullptr ) /*0x1803056bf*/
    *a6 = 1.0; /*0x1803056c1*/
  if ( a8 != nullptr ) /*0x1803056d2*/
    *a8 = 1.0; /*0x1803056d4*/
  result = a7; /*0x1803056da*/
  if ( a7 != nullptr ) /*0x1803056e5*/
LABEL_25:
    *result = 1.0; /*0x1803056e7*/
  return result; /*0x1803056f2*/
}
