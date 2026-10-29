__int64 __fastcall sub_1803520D0(__int64 a1, __int64 a2, _BYTE *a3)
{
  _BYTE *v3; // rbx
  __int64 v4; // rdi
  __int64 v5; // r9
  int v6; // edx
  unsigned __int8 *v7; // r8
  int v8; // r10d
  int v9; // r11d
  int v10; // esi
  int v11; // edi
  int v12; // ebx
  int v13; // r12d
  int v14; // r13d
  int v15; // r14d
  int v16; // r15d
  unsigned int v17; // eax
  unsigned int v18; // ecx
  unsigned int v19; // eax
  unsigned int v20; // ecx
  unsigned int v21; // eax
  unsigned int v22; // ecx
  unsigned int v23; // eax
  unsigned int v24; // ecx
  unsigned int v25; // eax
  unsigned int v26; // ecx
  unsigned int v27; // eax
  unsigned int v28; // eax
  unsigned int v29; // eax
  int v30; // eax
  int v31; // ecx
  _DWORD *v32; // rcx
  unsigned __int8 v33; // di
  unsigned int *v34; // r9
  __int64 i; // r11
  __int64 result; // rax
  unsigned int v37; // eax
  unsigned int v38; // edx
  unsigned int v39; // eax
  unsigned int v40; // edx
  unsigned int v41; // eax
  unsigned int v42; // edx
  unsigned int v43; // eax
  unsigned int v44; // edx
  unsigned int v45; // eax
  unsigned int v46; // edx
  unsigned int v47; // eax
  unsigned int v48; // edx
  int v49; // edx
  int v52; // [rsp+30h] [rbp-49h] BYREF
  int v53; // [rsp+34h] [rbp-45h]
  int v54; // [rsp+38h] [rbp-41h] BYREF
  int v55; // [rsp+3Ch] [rbp-3Dh]
  int v56; // [rsp+40h] [rbp-39h]
  int v57; // [rsp+44h] [rbp-35h]
  int v58; // [rsp+48h] [rbp-31h]
  int v59; // [rsp+4Ch] [rbp-2Dh]
  int v60; // [rsp+50h] [rbp-29h]
  int v61; // [rsp+54h] [rbp-25h]
  int v62; // [rsp+58h] [rbp-21h]
  int v63; // [rsp+5Ch] [rbp-1Dh]
  int v64; // [rsp+60h] [rbp-19h]
  int v65; // [rsp+64h] [rbp-15h]
  int v66; // [rsp+68h] [rbp-11h]
  int v67; // [rsp+6Ch] [rbp-Dh]
  int v68; // [rsp+70h] [rbp-9h]
  int v69; // [rsp+74h] [rbp-5h]
  int v70; // [rsp+78h] [rbp-1h]
  int v71; // [rsp+7Ch] [rbp+3h]
  int v72; // [rsp+80h] [rbp+7h]
  int v73; // [rsp+84h] [rbp+Bh]
  int v74; // [rsp+88h] [rbp+Fh]
  int v75; // [rsp+8Ch] [rbp+13h]
  int v76; // [rsp+90h] [rbp+17h]
  int v77; // [rsp+94h] [rbp+1Bh]
  int v78; // [rsp+98h] [rbp+1Fh]
  int v79; // [rsp+9Ch] [rbp+23h]

  v3 = a3; /*0x1803520ef*/
  v4 = a2; /*0x1803520f6*/
  sub_180102B80(a1: &v52, a2: 0, a3: 112); /*0x180352107*/
  v5 = (unsigned __int8)*v3; /*0x18035210c*/
  if ( *v3 != 0 ) /*0x180352113*/
  {
    v6 = v79; /*0x180352119*/
    v7 = v3 + 4; /*0x18035211c*/
    v8 = v78; /*0x180352120*/
    v9 = v77; /*0x180352124*/
    v10 = v76; /*0x180352128*/
    v11 = v71; /*0x18035212b*/
    v12 = v70; /*0x18035212e*/
    v13 = v73; /*0x180352139*/
    v14 = v72; /*0x180352145*/
    v15 = v75; /*0x180352151*/
    v16 = v74; /*0x18035215d*/
    do /*0x18035226e*/
    {
      v17 = *(v7 - 1); /*0x180352166*/
      v53 += v17 & 0xF; /*0x18035216b*/
      v18 = *v7; /*0x18035216e*/
      v52 += v17 >> 4; /*0x180352175*/
      v55 += v18 & 0xF; /*0x180352180*/
      v54 += v18 >> 4; /*0x180352183*/
      v19 = v7[1]; /*0x18035218b*/
      v57 += v19 & 0xF; /*0x180352190*/
      v20 = v7[2]; /*0x180352193*/
      v56 += v19 >> 4; /*0x18035219b*/
      v59 += v20 & 0xF; /*0x1803521a6*/
      v58 += v20 >> 4; /*0x1803521a9*/
      v21 = v7[3]; /*0x1803521b1*/
      v61 += v21 & 0xF; /*0x1803521b6*/
      v22 = v7[4]; /*0x1803521b9*/
      v60 += v21 >> 4; /*0x1803521c1*/
      v63 += v22 & 0xF; /*0x1803521cc*/
      v62 += v22 >> 4; /*0x1803521cf*/
      v23 = v7[5]; /*0x1803521d7*/
      v65 += v23 & 0xF; /*0x1803521dc*/
      v24 = v7[6]; /*0x1803521df*/
      v64 += v23 >> 4; /*0x1803521e7*/
      v67 += v24 & 0xF; /*0x1803521f2*/
      v66 += v24 >> 4; /*0x1803521f5*/
      v25 = v7[7]; /*0x1803521fd*/
      v69 += v25 & 0xF; /*0x180352202*/
      v26 = v7[8]; /*0x180352205*/
      v68 += v25 >> 4; /*0x18035220d*/
      v11 += v26 & 0xF; /*0x180352218*/
      v12 += v26 >> 4; /*0x18035221a*/
      v27 = v7[9]; /*0x180352221*/
      v13 += v27 & 0xF; /*0x180352229*/
      v14 += v27 >> 4; /*0x180352231*/
      v28 = v7[10]; /*0x180352234*/
      v15 += v28 & 0xF; /*0x18035223c*/
      v16 += v28 >> 4; /*0x180352244*/
      v29 = v7[11]; /*0x180352247*/
      v9 += v29 & 0xF; /*0x18035224f*/
      v10 += v29 >> 4; /*0x180352257*/
      v30 = v7[12] >> 4; /*0x18035225b*/
      v31 = v7[12] & 0xF; /*0x18035225e*/
      v7 += 16; /*0x180352261*/
      v6 += v31; /*0x180352265*/
      v8 += v30; /*0x180352267*/
      --v5; /*0x18035226a*/
    }
    while ( v5 != 0 ); /*0x18035226e*/
    v75 = v15; /*0x180352274*/
    v74 = v16; /*0x180352280*/
    v73 = v13; /*0x18035228c*/
    v72 = v14; /*0x180352298*/
    v71 = v11; /*0x1803522a4*/
    v4 = a2; /*0x1803522a7*/
    v70 = v12; /*0x1803522ab*/
    v3 = a3; /*0x1803522ae*/
    v79 = v6; /*0x1803522b2*/
    v78 = v8; /*0x1803522b5*/
    v77 = v9; /*0x1803522b9*/
    v76 = v10; /*0x1803522bd*/
  }
  v32 = (_DWORD *)(v4 + 20); /*0x1803522c2*/
  v33 = byte_1812D3E77; /*0x1803522c6*/
  v34 = (unsigned int *)&v54; /*0x1803522cd*/
  for ( i = 4; i != 0; --i ) /*0x1803522d1*/
  {
    result = (unsigned __int8)*v3; /*0x1803522d5*/
    if ( (_BYTE)result != 0 ) /*0x1803522da*/
    {
      v37 = *(v34 - 2) / (unsigned __int8)result; /*0x1803522ea*/
      *(v34 - 2) = v37; /*0x1803522f1*/
      v38 = v37; /*0x1803522f5*/
      if ( v37 < v33 ) /*0x1803522fa*/
      {
        *(v34 - 2) = 0; /*0x1803522fc*/
        v38 = 0; /*0x180352300*/
      }
      *(v32 - 5) = v37; /*0x180352302*/
      *(v32 - 4) = v38; /*0x180352305*/
      if ( v38 != 0 ) /*0x18035230a*/
        ++*(v32 - 3); /*0x18035230c*/
      else
        *(v32 - 3) = 0; /*0x180352311*/
      result = (unsigned __int8)*v3; /*0x180352314*/
      if ( (_BYTE)result != 0 ) /*0x180352319*/
      {
        v39 = *(v34 - 1) / (unsigned __int8)result; /*0x180352329*/
        *(v34 - 1) = v39; /*0x18035232c*/
        v40 = v39; /*0x180352330*/
        if ( v39 < v33 ) /*0x180352335*/
        {
          *(v34 - 1) = 0; /*0x180352337*/
          v40 = 0; /*0x18035233b*/
        }
        *(v32 - 2) = v39; /*0x18035233d*/
        *(v32 - 1) = v40; /*0x180352340*/
        if ( v40 != 0 ) /*0x180352345*/
          ++*v32; /*0x180352347*/
        else
          *v32 = 0; /*0x18035234b*/
        result = (unsigned __int8)*v3; /*0x18035234d*/
        if ( (_BYTE)result != 0 ) /*0x180352352*/
        {
          v41 = *v34 / (unsigned __int8)result; /*0x180352361*/
          *v34 = v41; /*0x180352364*/
          v42 = v41; /*0x180352367*/
          if ( v41 < v33 ) /*0x18035236c*/
          {
            *v34 = 0; /*0x18035236e*/
            v42 = 0; /*0x180352371*/
          }
          v32[1] = v41; /*0x180352373*/
          v32[2] = v42; /*0x180352376*/
          if ( v42 != 0 ) /*0x18035237b*/
            ++v32[3]; /*0x18035237d*/
          else
            v32[3] = 0; /*0x180352382*/
          result = (unsigned __int8)*v3; /*0x180352385*/
          if ( (_BYTE)result != 0 ) /*0x18035238a*/
          {
            v43 = v34[1] / (unsigned __int8)result; /*0x18035239a*/
            v34[1] = v43; /*0x18035239d*/
            v44 = v43; /*0x1803523a1*/
            if ( v43 < v33 ) /*0x1803523a6*/
            {
              v34[1] = 0; /*0x1803523a8*/
              v44 = 0; /*0x1803523ac*/
            }
            v32[4] = v43; /*0x1803523ae*/
            v32[5] = v44; /*0x1803523b1*/
            if ( v44 != 0 ) /*0x1803523b6*/
              ++v32[6]; /*0x1803523b8*/
            else
              v32[6] = 0; /*0x1803523bd*/
            result = (unsigned __int8)*v3; /*0x1803523c0*/
            if ( (_BYTE)result != 0 ) /*0x1803523c5*/
            {
              v45 = v34[2] / (unsigned __int8)result; /*0x1803523d5*/
              v34[2] = v45; /*0x1803523d8*/
              v46 = v45; /*0x1803523dc*/
              if ( v45 < v33 ) /*0x1803523e1*/
              {
                v34[2] = 0; /*0x1803523e3*/
                v46 = 0; /*0x1803523e7*/
              }
              v32[7] = v45; /*0x1803523e9*/
              v32[8] = v46; /*0x1803523ec*/
              if ( v46 != 0 ) /*0x1803523f1*/
                ++v32[9]; /*0x1803523f3*/
              else
                v32[9] = 0; /*0x1803523f8*/
              result = (unsigned __int8)*v3; /*0x1803523fb*/
              if ( (_BYTE)result != 0 ) /*0x180352400*/
              {
                v47 = v34[3] / (unsigned __int8)result; /*0x18035240c*/
                v34[3] = v47; /*0x18035240f*/
                v48 = v47; /*0x180352413*/
                if ( v47 < v33 ) /*0x180352418*/
                {
                  v34[3] = 0; /*0x18035241a*/
                  v48 = 0; /*0x18035241e*/
                }
                v32[10] = v47; /*0x180352420*/
                v32[11] = v48; /*0x180352423*/
                if ( v48 != 0 ) /*0x180352428*/
                  ++v32[12]; /*0x18035242a*/
                else
                  v32[12] = 0; /*0x18035242f*/
                result = (unsigned __int8)*v3; /*0x180352432*/
                if ( (_BYTE)result != 0 ) /*0x180352437*/
                {
                  result = v34[4] / (unsigned __int8)result; /*0x180352443*/
                  v34[4] = result; /*0x180352446*/
                  v49 = result; /*0x18035244a*/
                  if ( (unsigned int)result < v33 ) /*0x18035244f*/
                  {
                    v34[4] = 0; /*0x180352451*/
                    v49 = 0; /*0x180352455*/
                  }
                  v32[13] = result; /*0x180352457*/
                  v32[14] = v49; /*0x18035245a*/
                  if ( v49 != 0 ) /*0x18035245f*/
                    ++v32[15]; /*0x180352461*/
                  else
                    v32[15] = 0; /*0x180352466*/
                }
              }
            }
          }
        }
      }
    }
    v34 += 7; /*0x180352469*/
    v32 += 21; /*0x18035246d*/
  }
  return result; /*0x18035247b*/
}
