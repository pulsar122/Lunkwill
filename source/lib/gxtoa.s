; @(#)gxtoa.s, Dirk Haun, 12.11.1991
; gttoa() & gdtoa - Gemdos Time/Date to ASCII

               export    gttoa, gdtoa

; void gttoa(unsigned int g[D0],char *a[A0]);
;
; wandelt eine Gemdos-Zeitangabe (RÅckgabe von Tgettime())
; in einen ASCII-String hh:mm:ss. 'a' muû daher Platz fÅr die
; acht Zeichen plus Nullbyte aufweisen.

module         gttoa
               moveq     #'0',D2
               moveq     #0,D1
               move.w    D0,D1
               lsr.w     #8,D1
               lsr.w     #3,D1
               divu      #10,D1
               add.b     D2,D1
               move.b    D1,(A0)+
               swap      D1
               add.b     D2,D1
               move.b    D1,(A0)+
               move.b    #':',(A0)+
               moveq     #0,D1
               move.w    D0,D1
               and.w     #$07e0,D1
               lsr.w     #5,D1
               divu      #10,D1
               add.b     D2,D1
               move.b    D1,(A0)+
               swap      D1
               add.b     D2,D1
               move.b    D1,(A0)+
               move.b    #':',(A0)+
               moveq     #0,D1
               move.w    D0,D1
               and.w     #$1f,D1
               lsl.w     #1,D1
               divu      #10,D1
               add.b     D2,D1
               move.b    D1,(A0)+
               swap      D1
               add.b     D2,D1
               move.b    D1,(A0)+
               clr.b     (A0)
               rts
endmod


; void gdtoa(unsigned int d,char *a);
;
; Wandelt eine Gemdos-Datumsangabe (RÅckgabe von Tgetdate())
; in einen ASCII-String tt:mm:jjjj. 'a' muû daher Platz fÅr
; zehn Zeichen plus Nullbyte aufweisen.

               import    itoa

module         gdtoa
               moveq     #'0',D2
               moveq     #0,D1
               move.w    D0,D1
               and.w     #$1f,D1
               divu      #10,D1
               add.b     D2,D1
               move.b    D1,(A0)+
               swap      D1
               add.b     D2,D1
               move.b    D1,(A0)+
               move.b    #'.',(A0)+
               moveq     #0,D1
               move.w    D0,D1
               and.w     #$1e0,D1
               lsr.w     #5,D1
               divu      #10,D1
               add.b     D2,D1
               move.b    D1,(A0)+
               swap      D1
               add.b     D2,D1
               move.b    D1,(A0)+
               move.b    #'.',(A0)+
               and.w     #$fe00,D0
               lsr.w     #8,D0
               lsr.w     #1,D0
               add.w     #1980,D0
               moveq     #10,D1
               jsr       itoa
               rts
endmod
