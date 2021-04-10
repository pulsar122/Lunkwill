/*
  match()
  -------

   Matched den string mit dem pattern nach dem Unix-match
   fr Dateinamen.
   Rckgabewert ist   1 fr Erfolg
                      0 fr Mierfolg
   Beachtet werden *, ?, [xyz], [!yxc], [x-y]

   Ein gutes Beispiel fr kommentarlosen, unverst„ndlichen Code ;-)

   match() ist trotz einer durch goto aufgel”sten Rekursion
   immernoch stark rekursiv. (Achtung auf den Stack ,-)

   Autor  : Wolfgang_Wander@lu.maus.de
   Status : Public Domain

   Diese Funktion wurde ohne Einblick in entsprechende Sourcen
   aus z.B. Unix-Libraries im Jahr 1988 entwickelt und ist
   dementsprechend frei von Rechten Dritter.

   Dieser Header darf nicht entfernt oder verkrzt werden.
*/

int match (char *pattern, char *string) {
start:
   if(! *string) {
   if(!*pattern)
         return 1;
   else if(*pattern=='*') {
         pattern ++;
         goto start;
      }
   } else {
      if(*pattern == '*')
         return match(pattern+1,string) || match(pattern,string+1);
      else if(*pattern == '[') {
         char suc = 0 ,not = 0;
         pattern ++;
         if(*pattern == '^' || *pattern == '!')
            not = 1,pattern++;
         while(*pattern && (*pattern != ']' ||
              (*pattern == ']' && pattern[1] == ']') ||
                pattern[-1] == '[' )) {
            if(*pattern == '-' && pattern[-1] != '[' && pattern[1] != ']')
               suc |= *(pattern-1) <= *string && *string <= pattern[1];
            else
               suc |= *pattern == *string;
            pattern++;
         }
         if(suc ^ not) {
            if(*pattern)
                  pattern++;
            string ++;
            goto start;
         }
      } else if(*pattern == *string || *pattern == '?') {
         pattern ++;
         string++;
         goto start;
      }
   }
   return 0;
}
