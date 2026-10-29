void __fastcall CLibXact::SetSoundWithParam(
        CLibXact *this,
        __int64 a2,
        float a3,
        float a4,
        float a5,
        float a6,
        bool a7)
{
  if ( *((_DWORD *)this + 145415) == 2 ) /*0x180008a1b*/
  {
    if ( a7 ) /*0x180008a43*/
      CLibXact::sync_wait( /*0x180008a75*/
        this,
        a2: 20,
        a3: 0,
        a4: 0,
        a5: a3,
        a6: a4,
        a7: a5,
        a8: a6,
        a9: a2,
        a10: nullptr,
        a11: nullptr,
        a12: a7);
    else
      CLibXact::nosync_nowait( /*0x180008ab0*/
        this,
        a2: 20,
        a3: 0,
        a4: 0,
        a5: a3,
        a6: a4,
        a7: a5,
        a8: a6,
        a9: a2,
        a10: nullptr,
        a11: nullptr);
  }
}
