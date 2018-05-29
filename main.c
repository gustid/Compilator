  #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define SAFEALLOC(var,Type) if ((var=(Type*)malloc( sizeof (Type)))== NULL )err( "not enough memory");
int unit();
int declStruct();
int declVar();
int declFunc();
int funcArg();
int stm();
int stmCompound();
/*int expr();
int exprAssign();
int exprOr();
int exprOr1();
int exprAnd();
int exprAnd1();
int exprEq();
int exprEq1();
int exprRel();
int exprRel1();
int exprAdd();
int exprAdd1();
int exprMul();
int exprMul1();
int exprCast();
int exprUnary();
int exprPostfix();
int exprPostfix1();
int exprPrimary();
int exprPrimary1();
int exprPrimary2();
*/

char *pch;
char buffer[30001];
int line=0;
FILE *g;

enum {ID , CT_INT, CT_REAL, CT_CHAR, CT_STRING,
    COMMA , SEMICOLON , LPAR , RPAR , LBRACKET , RBRACKET , LACC , RACC ,
    ADD, SUB, MUL, DIV, DOT , AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
    SPACE, LINECOMMENT, COMMENT,
    END, //codurile AL
    BREAK , CHAR , DOUBLE , ELSE , FOR , IF , INT , RETURN , STRUCT , VOID , WHILE
};

//_______________analiza de domeniu_____________________________________________________________________________________

  enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
  enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
  enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

//_______________analiza de domeniu_____________________________________________________________________________________

char *names[]= {"ID" , "CT_INT", "CT_REAL", "CT_CHAR", "CT_STRING",
    "COMMA" , "SEMICOLON" , "LPAR" , "RPAR" , "LBRACKET" , "RBRACKET" , "LACC" , "RACC" ,
    "ADD", "SUB", "MUL", "DIV", "DOT" , "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ",
    "SPACE", "LINECOMMENT", "COMMENT",
    "END", //codurile AL
    "BREAK" , "CHAR" , "DOUBLE" , "ELSE" , "FOR" , "IF" , "INT" , "RETURN" , "STRUCT" , "VOID" , "WHILE"
};

//enum {BREAK , CHAR , DOUBLE , ELSE , FOR , IF , INT , RETURN , STRUCT , VOID , WHILE };

typedef struct _Token{
    int code;         //codul(numele)
    union{
        char *text;   //folosit pentru ID, CT_STRING(alocat dinamic)
        int i;   //folosit pentru CT_INT,CT_CHAR
        double r;     //folosit pentru CT_REAL
    };

    int line;    //linia din fisierul de intrare
    struct _Token *next ;    //inlantuire la urmatorul AL

}Token;

  Token *crtTk;
  Token *consumedTk;

//_______________analiza de domeniu_____________________________________________________________________________________
  struct _Symbol;
  typedef struct _Symbol Symbol;

  typedef struct
  {
      Symbol **begin; // the beginning of the symbols, or NULL
      Symbol **end; // the position after the last symbol
      Symbol **after; // the position after the allocated space
  }Symbols;

  typedef struct
  {
      int typeBase; // TB_*
      Symbol *s; // struct definition for TB_STRUCT
      int nElements; // >0 array of given size, 0=array without size, <0 non array
  }Type;

  typedef struct _Symbol
  {
      const char *name; // a reference to the name stored in a token
      int cls; // CLS_*
      int mem; // MEM_*
      Type type;
      int depth; // 0-global, 1-in function, 2... - nested blocks in function
      union
      {
          Symbols args; // used only of functions
          Symbols members; // used only for structs
      };
  }Symbol;

  int crtDepth = 0;
  Symbols *pSymbolCrt = NULL;
  Symbol *crtStruct = NULL;
  Symbol *crtFunc = NULL;
  Symbols symbols;

  int typeBase(Type *);
  int arrayDecl(Type *);
  int typeName(Type *);


//_______________analiza de domeniu_____________________________________________________________________________________

  void err(const char *fmt,...)
  {
      va_list va;
      va_start(va,fmt);
      fprintf(stderr,"error: ");
      vfprintf(stderr,fmt,va);
      fputc('\n',stderr);
      va_end(va);
      exit(-1);
  }

  Token *lastToken = NULL;
  Token *tokens = NULL;

  Token *addTk(int code)
  {
      Token *tk;
      SAFEALLOC(tk,Token);
      tk->code=code;
      tk->line=line;
      tk->next=NULL;
      if(lastToken){
          lastToken->next=tk;
      }else{
          tokens=tk;
      }
      lastToken=tk;
      return tk;
  }



  void tkerr(const Token* tk, const char* fmt,...)
  {
      va_list va;
      va_start(va,fmt);
      fprintf(stderr,"error in line %d: ",tk->line);
      vfprintf(stderr,fmt,va);
      fputc('\n',stderr);
      va_end(va);
      exit(-1);
  }

  char escChar(char ch)
  {
      switch(ch){
          case 'a' : return '\a';
          case 'b' : return '\b';
          case 'f' : return '\f';
          case 'n' : return '\n';
          case 'r' : return '\r';
          case 't' : return '\t';
          case 'v' : return '\v';
          case '\'' : return '\'';
          case '?' : return '\?';
          case '"' : return '\"';
          case '\\' : return '\\';
          case '0' : return '\0';
          default : return ch;
      }
  }

  char * createString(char* start, char* stop)
  {
      int n = stop-start;
      char *word ;
      word = (char *)malloc((n+1)*sizeof(char));
      memcpy(word,start,n);
      word[n]='\0';
      int i;
      for(i=0;i<strlen(word);i++)
      {
          if(word[i]=='\\'){word[i]=escChar(word[i+1]);strcpy(word+(i+1),word+(i+2));}

      }

      return word;

  }

  void showAtoms()
  {
      Token *q = tokens;
      while(q!=NULL)
      {
          printf(" %s ",names[q->code]);
          if(!strcmp(names[q->code],"CT_INT")) printf(" : %d ",q->i);
          if(!strcmp(names[q->code],"CT_REAL"))printf(" : %f ",q->r);
          if(!strcmp(names[q->code],"ID"))printf(" : %s ",q-> text);
          if(!strcmp(names[q->code],"CT_CHAR"))printf(" : %c ",q->i);
          if(!strcmp(names[q->code],"CT_STRING")){
              /*int i;
              printf(" : ");
              for(i=0;i<strlen(q->text);i++)
              {
                  if(q->text[i]=='\\'){printf("%c",escChar(q->text[i+1]));i++;}
                  else printf("%c",q->text[i]);

              }*/
              printf(" : %s ",q->text);
          }
          printf(" line - %d ",q->line);
          printf("\n");
          q=q->next;

      }
  }
//_______________analiza tipurilor______________________________________________________________________________________

  Type createType(int typeBase,int nElements)
  {
      Type t;
      t.typeBase=typeBase;
      t.nElements=nElements;
      return t;
  }
  typedef union{
      long int i; // int, char
      double d; // double
      const char *str; // char[]
  }CtVal;
  typedef struct{
      Type type; // type of the result
      int isLVal; // if it is a LVal
      int isCtVal; // if it is a constant value (int, real, char, char[])
      CtVal ctVal; // the constat value
  }RetVal;

  void cast(Type *dst,Type *src) {
      if (src->nElements > -1) {
          if (dst->nElements > -1) {
              if (src->typeBase != dst->typeBase)
                  tkerr(crtTk, "an array cannot be converted to an array of another type");
          } else {
              tkerr(crtTk, "an array cannot be converted to a non-array");
          }
      } else {
          if (dst->nElements > -1) {
              tkerr(crtTk, "a non-array cannot be converted to an array");
          }
      }
      switch (src->typeBase) {
          case TB_CHAR:
          case TB_INT:
          case TB_DOUBLE:

              switch (dst->typeBase) {
                  case TB_CHAR:
                  case TB_INT:
                  case TB_DOUBLE:
                      return;
              }
          case TB_STRUCT:
              if (dst->typeBase == TB_STRUCT) {
                  if (src->s != dst->s)
                      tkerr(crtTk, "a structure cannot be converted to another one");
                  return;
              }
      }
      tkerr(crtTk, "incompatible types");
  }

  Type getArithType(Type *s1, Type *s2)
  {
      switch (s1->typeBase)
      {
          case TB_CHAR:
              return *s2;
              break;
          case TB_INT:
              switch (s2->typeBase)
              {
                  case TB_CHAR: return *s1;
                      break;
                  case TB_INT: return *s1;
                      break;
                  case TB_DOUBLE: return *s2;
                      break;
              }
              break;
          case TB_DOUBLE:
              switch (s2->typeBase)
              {
                  case TB_CHAR: return *s1;
                      break;
                  case TB_INT: return *s1;
                      break;
                  case TB_DOUBLE: return *s1;
                      break;
              }
              break;
      }
  }
  int expr(RetVal*);
  int exprAssign(RetVal*);
  int exprOr(RetVal*);
  int exprOr1(RetVal*);
  int exprAnd(RetVal*);
  int exprAnd1(RetVal*);
  int exprEq(RetVal*);
  int exprEq1(RetVal*);
  int exprRel(RetVal*);
  int exprRel1(RetVal*);
  int exprAdd(RetVal*);
  int exprAdd1(RetVal*);
  int exprMul(RetVal*);
  int exprMul1(RetVal*);
  int exprCast(RetVal*);
  int exprUnary(RetVal*);
  int exprPostfix(RetVal*);
  int exprPostfix1(RetVal*);
  int exprPrimary(RetVal*);



//_______________analiza tipurilor______________________________________________________________________________________


//_______________analiza de domeniu_____________________________________________________________________________________

  void initSymbols(Symbols *symbols)
  {
      symbols->begin = NULL;
      symbols->end = NULL;
      symbols->after = NULL;
  }

  Symbol *addSymbol(Symbols *symbols, const char *name, int cls)
  {
      Symbol *s;
      if (symbols->end == symbols->after)
      { // create more room
          int count = symbols->after - symbols->begin;
          int n = count * 2; // double the room
          if (n == 0) n = 1; // needed for the initial case
          symbols->begin = (Symbol**)realloc(symbols->begin, n * sizeof(Symbol*));
          if (symbols->begin == NULL)err("not enough memory");
          symbols->end = symbols->begin + count;
          symbols->after = symbols->begin + n;
      }
      SAFEALLOC(s, Symbol)
      *symbols->end++ = s;
      s->name = name;
      s->cls = cls;
      s->depth = crtDepth;
      return s;
  }

  Symbol *findSymbol(Symbols *symbol, const char *name)
  {
      if (symbol->begin == NULL) return NULL;

      Symbol* aux = *symbol->begin;

      int i = symbol->end - symbol->begin;
      i--;


      while (i >= 0)
      {
          aux = symbol->begin[i];
          if (!strcmp(aux->name, name))
              return aux;
          i--;
      }

      return NULL;
  }

  void deleteSymbolsAfter(Symbols* symbols, Symbol* start)
  {
      Symbol *c = NULL;
      if ((c = findSymbol(symbols, start->name)) != NULL)
      {
          *symbols->end--;
          while (c != *symbols->end)
          {
              free(*symbols->end--);
          }
          *symbols->end++;
      }
      else
          printf("Nu exista Simbolul dat ca start\n");
  }

  void        addVar(Token *tkName,Type *t)
  {
      Symbol      *s;
      //printf("#%s %d \n",tkName->text,crtDepth);
      if(crtStruct){

          if(findSymbol(&crtStruct->members,tkName->text))
              tkerr(crtTk,"symbol redefinition: %s",tkName->text);
          s=addSymbol(&crtStruct->members,tkName->text,CLS_VAR);
      }
      else if(crtFunc){
          s=findSymbol(&symbols,tkName->text);
          if(s&&s->depth==crtDepth)
              tkerr(crtTk,"symbol redefinition: %s",tkName->text);
          s=addSymbol(&symbols,tkName->text,CLS_VAR);
          s->mem=MEM_LOCAL;
      }
      else{
          if(findSymbol(&symbols,tkName->text))
              tkerr(crtTk,"symbol redefinition: %s",tkName->text);
          s=addSymbol(&symbols,tkName->text,CLS_VAR);
          s->mem=MEM_GLOBAL;
      }
      s->type=*t;
  }
//_______________analiza de domeniu_____________________________________________________________________________________
  int getNextToken()
{
    int s=0;          //variable used for the current state
    char ch;
    char *pStartCh;
    int n;
    Token *tk;

    for(;;){
        ch=*pch;
        //printf("#starea %d cu %c(%d) \n", s, ch,ch);
        //fprintf(g,"#starea %d cu %c(%d) \n", s, ch,ch);

        switch(s){
            case 0:
                if(isalpha(ch) || ch == '_') {pStartCh = pch; s=1;pch++;}
                else if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ) {pch++;}
                if(ch == '\n'){line++;}

                else if(isdigit(ch) && ch != '0'){pStartCh = pch;s=6;pch++;}

                else if(ch == '0'){pStartCh = pch;s=19;pch++;}

                else if(ch == '='){s=4;pch++;}

                else if(ch == ','){s=11;pch++;}
                else if(ch == ';'){s=12;pch++;}
                else if(ch == '('){s=13;pch++;}
                else if(ch == ')'){s=14;pch++;}
                else if(ch == '['){s=15;pch++;}
                else if(ch == ']'){s=16;pch++;}
                else if(ch == '{'){s=17;pch++;}
                else if(ch == '}'){s=18;pch++;}

                else if(ch == '+'){s=20;pch++;}
                else if(ch == '-'){s=21;pch++;}
                else if(ch == '*'){s=22;pch++;}
                else if(ch == '/'){s=23;pch++;}
                else if(ch == '&'){s=25;pch++;}
                else if(ch == '|'){s=27;pch++;}
                else if(ch == '!'){s=30;pch++;}
                else if(ch == '<'){s=32;pch++;}
                else if(ch == '>'){s=37;pch++;}
                else if(ch == '.'){s=38;pch++;}

                else if(ch == '"'){pStartCh = pch;s=56;pch++;}
                else if(ch == '\''){pStartCh = pch;s=52;pch++;}
                else if(ch == '\0'){s=59;pch++;}
                break;
            case 1:
                if(isalnum(ch)||ch == '_'){pch++;}
                else {s=2;}
				break;
			case 2:
                n = pch - pStartCh;
                if(!memcmp(pStartCh,"break",n)&&n==5){addTk(BREAK);return BREAK;}
                else if(!memcmp(pStartCh,"char",n)&&n==4){addTk(CHAR);return CHAR;}
                else if(!memcmp(pStartCh,"double",n)&&n==6){addTk(DOUBLE);return DOUBLE;}
				else if(!memcmp(pStartCh,"else",n)&&n==4){addTk(ELSE);return ELSE;}
				else if(!memcmp(pStartCh,"for",n)&&n==3){addTk(FOR);return FOR;}
				else if(!memcmp(pStartCh,"if",n)&&n==2){addTk(IF);return IF;}
				else if(!memcmp(pStartCh,"int",n)&&n==3){addTk(INT);return INT;}
				else if(!memcmp(pStartCh,"return",n)&&n==6){addTk(RETURN);return RETURN;}
				else if(!memcmp(pStartCh,"struct",n)&&n==6){addTk(STRUCT);return STRUCT;}
				else if(!memcmp(pStartCh,"void",n)&&n==4){addTk(VOID);return VOID;}
				else if(!memcmp(pStartCh,"while",n)&&n==5){addTk(WHILE);return WHILE;}
				else {
					tk = addTk(ID);
					tk->text = createString(pStartCh,pch);
					//printf("@%s \n", tk->text);
					//................
					return ID;
				}
				break;
			case 3:
				addTk(ASSIGN);
				return ASSIGN;
				break;
			case 4:
				if(ch == '='){s=5;pch++;}
				else {s=3;}
				break;
			case 5:
				addTk(EQUAL);
				return EQUAL;
				break;
			case 6:
				if(isdigit(ch)){pch++;}
				else if(ch == '.'){s=42;pch++;}
				else if(ch == 'e'){s=45;pch++;}
				else if(ch == 'E'){s=46;pch++;}
				else {s=7;}
				break;
			case 7:
				n = pch - pStartCh;
				tk = addTk(CT_INT);
				char *temp = createString(pStartCh,pch);
				if(temp[0]=='0'){
					if(temp[1]=='x'){
                        tk->i = strtol(temp,NULL,16);
                    }else{
                    tk->i = strtol(temp,NULL,8);
                    }
                    }else{
					tk->i = strtol(temp,NULL,10);
                    }
				return CT_INT;
				break;
			case 8:
				if(ch>='0' && ch<='7'){pch++;}
				else{s=7;}
				break;
			case 9:
				if(isdigit(ch)||(ch>='A' && ch<='F')||(ch>='a'&&ch<='f')){s=10;pch++;}
				break;
			case 10:
				if(isdigit(ch)||(ch>='A' && ch<='F')||(ch>='a'&&ch<='f')){pch++;}
				else {s=7;}
				break;
			case 11:
				addTk(COMMA);
				return COMMA;
				break;
			case 12:
				addTk(SEMICOLON);
				return SEMICOLON;
				break;
			case 13:
				addTk(LPAR);
				return LPAR;
				break;
			case 14:
				addTk(RPAR);
				return RPAR;
				break;
			case 15:
				addTk(LBRACKET);
				return LBRACKET;
				break;
			case 16:
				addTk(RBRACKET);
				return RBRACKET;
				break;
			case 17:
				addTk(LACC);
				return LACC;
				break;
			case 18:
				addTk(RACC);
				return RACC;
				break;
			case 19:
				if(ch<='7' && ch>='0'){s=8;pch++;}
				else if(ch == 'x'){s=9;pch++;}
				else if(ch == '.'){s=42;pch++;}
				else if(ch == 'e'){s=45;pch++;}
				else if(ch == 'E'){s=46;pch++;}
				else {s=7;}
				break;
			case 20:
				addTk(ADD);
				return ADD;
				break;
			case 21:
				addTk(SUB);
				return SUB;
				break;
			case 22:
				addTk(MUL);
				return MUL;
				break;
			case 23:
				if(ch == '/'){s=39;pch++;}
				else if(ch == '*'){s=40;pch++;}
				else {s=24;}
				break;
			case 24:
				addTk(DIV);
				return DIV;
				break;
			case 25:
				if(ch == '&'){s=26;pch++;}
				break;
			case 26:
				addTk(AND);
				return AND;
				break;
			case 27:
				if(ch == '|'){s=28;pch++;}
				break;
			case 28:
				addTk(OR);
				return OR;
				break;
			case 29:
				addTk(NOT);
				return NOT;
				break;
			case 30:
				if(ch == '='){s=31;pch++;}
				else {s=29;}
				break;
			case 31:
				addTk(NOTEQ);
				return NOTEQ;
				break;
			case 32:
				if(ch == '='){s=34;pch++;}
				else {s=33;}
				break;
			case 33:
				addTk(LESS);
				return LESS;
				break;
			case 34:
				addTk(LESSEQ);
				return LESSEQ;
				break;
			case 35:
				addTk(GREATER);
				return GREATER;
				break;
			case 36:
				addTk(GREATEREQ);
				return GREATEREQ;
				break;
			case 37:
				if(ch == '='){s=36;pch++;}
				else{s=35;}
				break;
			case 38:
				addTk(DOT);
				return DOT;
				break;
			case 39:
				if(ch != '\n' && ch != '\r' && ch != '\0' ){s=39;pch++;}
				else{s=0;}
				break;
			case 40:
				if(ch != '*'){pch++;if(ch == '\n') line++;}
				else if(ch == '*'){s=41;pch++;}
				break;
			case 41:
				if(ch == '*'){s=41;pch++;}
				else if(ch == '/'){s=0;pch++;}
				else {s=40;pch++;if(ch=='\n')line++;}
				break;
			case 42:
				if(isdigit(ch)){s=43;pch++;}
				break;
            case 43:
                if(isdigit(ch)){s=43;pch++;}
                else if(ch == 'e'){s=45;pch++;}
                else if(ch == 'E'){s=46;pch++;}
                else {s=51;}
                break;
            case 44:
                n = pch - pStartCh ;
				tk = addTk(CT_STRING);
				tk->text = createString(pStartCh,pch);
				return CT_STRING;
				break;
            case 45:
                if(ch == '-'){s=47;pch++;}
                else if(ch == '+'){s=48;pch++;}
                else {s=49;}
                break;
            case 46:
                if(ch == '-'){s=47;pch++;}
                else if(ch == '+'){s=48;pch++;}
                else {s=49;}
                break;
            case 47:
                if(isdigit(ch)){s=50;pch++;}
				break;
            case 48:
                if(isdigit(ch)){s=50;pch++;}
				break;
            case 49:
                if(isdigit(ch)){s=50;pch++;}
				break;
            case 50:
                if(isdigit(ch)){s=50;pch++;}
                else {s=51;}
                break;
            case 51:
                n = pch - pStartCh;
				tk = addTk(CT_REAL);
				tk->r = atof(createString(pStartCh,pch));
				return CT_REAL;
				break;
            case 52:
                if(ch == '\\'){s=53;pch++;}
                else if(ch != '\\' && ch != '\''){s=54;pch++;}
                break;
            case 53:
                if(ch == 'a' || ch == 'b' || ch == 'f' || ch == 'n' || ch == 'r' || ch == 't' || ch == 'v' || ch == '\'' || ch == '?' || ch == '"' || ch == '\\' || ch == '0' ) {s=54;pch++;}
                break;
            case 54:
                if(ch == '\''){s=55;pch++;}
                break;
            case 55:
				n = pch - pStartCh;
				tk = addTk(CT_CHAR);
				char *tmp = createString(pStartCh,pch);
				if(tmp[1]=='\\')
				{
                    tk->i = escChar(tmp[2]);
				}else{
                    tk->i = tmp[1];
                }
				return CT_CHAR;
				break;
            case 56:
                if(ch == '\\'){s=57;pch++;}
                else if(ch != '\\' && ch != '"'){s=58;pch++;}
                break;
            case 57:
                if(ch == 'a' || ch == 'b' || ch == 'f' || ch == 'n' || ch == 'r' || ch == 't' || ch == 'v' || ch == '\'' || ch == '?' || ch == '"' || ch == '\\' || ch == '0' ) {s=58;pch++;}
                break;
            case 58:
                if(ch == '"'){s=44;pch++;}
                else {s=56;}
                break;
            case 59:
                addTk(END);
				return END;
				break;
            default :
                printf("stare nedeterminata %d \n",s);
        }//switch
    }//for

}//getNextToken

// -----------------------------------------------------sintactic-----------------------------------------------------------------------------



int consume(int code)
{

    if(crtTk->code == code)
    {
        printf("consume %s \n",names[crtTk->code]);
        consumedTk = crtTk;
        crtTk = crtTk->next;
        return 1; printf("%s \n",names[crtTk->code]);
    }
    else
    {
        return 0;
    }
}

int unit()
{
    Token *initTk = crtTk;
    printf("unit %s \n", names[crtTk->code]);
    while(1)
    {
        if(declStruct()){}
        else if (declFunc()){}
        else if (declVar()){}
        else break;
    }
    if(consume(END))
    {
        return 1; printf("%s \n",names[crtTk->code]);
    }else tkerr(crtTk,"nu e END");

    crtTk = initTk;
    return 0;
}

int declStruct()
{
    Token *initTk =  crtTk;
    Token *tkName = NULL;
    printf("declStruct %s \n", names[crtTk->code]);
    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            tkName = consumedTk ;
            if(consume(LACC))
            {

                    if(findSymbol(&symbols,tkName->text))
                        tkerr(crtTk,"symbol redefinition: %s",tkName->text);
                    crtStruct=addSymbol(&symbols,tkName->text,CLS_STRUCT);
                    initSymbols(&crtStruct->members);

                while(1)
                {
                    if(declVar()){}
                    else break;
                }
                if(consume(RACC))
                {
                    if(consume(SEMICOLON))
                    {
                        crtStruct = NULL;
                        return 1; printf("%s \n",names[crtTk->code]);
                    }else tkerr(crtTk,"missing ; declStruct");
                }else tkerr(crtTk,"missing }");
            }//else tkerr(crtTk,"missing { in declStruct");
        }else tkerr(crtTk,"missing ID in declStruct");
    }
    crtTk = initTk;
    return 0;
}

int declVar()
{
    Token *initTk = crtTk;
    printf("declVar %s \n", names[crtTk->code]);

    Type t;
    Token *tkName = NULL ;

    if(typeBase(&t))
    {
        if(consume(ID))
        {
            tkName = consumedTk;
            //if(!arrayDecl(&t))
                if (!arrayDecl(&t))
                {
                    t.nElements = -1;
                }
            addVar(tkName, &t);
            while(1)
            {
                if(consume(COMMA))
                {
                    if(consume(ID))
                    {
                        tkName = consumedTk;
                        //if(!arrayDecl(&t))
                            if (!arrayDecl(&t))
                            {
                                t.nElements = -1;
                            }
                        addVar(tkName, &t);
                    }else tkerr(crtTk,"missing ID in declVar ");
                }else break;
            }

            if(consume(SEMICOLON))
            {
                return 1;
            }else tkerr(crtTk,"missing ; declVar");
        }else tkerr(crtTk,"missing ID in declVar");
    }
    crtTk = initTk;
    return 0 ;
}


int typeBase(Type *ret)
{
    Token *initTk = crtTk;
    printf("typeBase %s \n", names[crtTk->code]);
    Token *tkName = NULL ;

    if(consume(INT))
    {
        ret->typeBase = TB_INT;
        return 1;
    }
    if(consume(DOUBLE))
    {
        ret->typeBase = TB_DOUBLE;
        return 1;
    }
    if(consume(CHAR))
    {
        ret->typeBase = TB_CHAR;
        return 1;
    }
    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            tkName = consumedTk;

                Symbol      *s=findSymbol(&symbols,tkName->text);
                if(s==NULL)tkerr(crtTk,"undefined symbol: %s",tkName->text);
                if(s->cls!=CLS_STRUCT)tkerr(crtTk,"%s is not a struct",tkName->text);
                ret->typeBase=TB_STRUCT;
                ret->s=s;

            return 1;
        }else tkerr(crtTk,"missing ID in typeBase");
    }
    crtTk = initTk;
    return 0;
}

int arrayDecl(Type *ret)
{
    Token *initTk = crtTk;
    printf("arrayDecl %s \n", names[crtTk->code]);
    RetVal rv;


    if(consume(LBRACKET))
    {
        if(expr(&rv))
        {
            if(!rv.isCtVal)tkerr(crtTk,"the array size is not a constant");
            if(rv.type.typeBase!=TB_INT)tkerr(crtTk,"the array size is not an integer");
            ret->nElements=rv.ctVal.i;
        }else
        {
            ret->nElements = 0;
        }
        if(consume(RBRACKET))
        {
            return 1; printf("%s \n",names[crtTk->code]);
        }else tkerr(crtTk,"missing ] in arrayDecl");
    }

    crtTk = initTk;
    return 0;
}

int typeName(Type *ret)
{
    Token *initTk = crtTk;
    printf("typeName %s \n", names[crtTk->code]);

    if(typeBase(ret))
    {
        if(!arrayDecl(ret))
        {
            ret->nElements = -1;
        }
        return 1;
    }

    crtTk = initTk;
    return 0;
}

int declFunc1(Type *t){

    Token *initTk = crtTk;
    printf("declFunc1 %s \n", names[crtTk->code]);

    if(typeBase(t))
    {
        if(consume(MUL))
        {
            t->nElements = 0;
        }else{
            t->nElements = -1;
        }
        return 1;
    }else if(consume(VOID))
    {
        t->nElements = TB_VOID;
        return 1;
    }

    crtTk = initTk;
    return 0;
}

int declFunc2()
{
    Token *initTk = crtTk;
    printf("declFunc2 %s \n", names[crtTk->code]);

    if(funcArg())
   {
       while(1)
       {
           if(consume(COMMA))
           {
               if(funcArg()){}
               else tkerr(crtTk, "missing funcArg after , in declFunc2");
           }
           else break;
           // printf("%s \n",names[crtTk->code]);
       }
       return 1;
   }

    crtTk = initTk;
    return 0;
}

int declFunc()
{
    Token *initTk = crtTk;
    printf("declFunc %s \n", names[crtTk->code]);
    Type t ;
    Token *tkName = NULL;

    if(declFunc1(&t)) {
        if (consume(ID))
        {
            tkName = consumedTk;
            if(consume(LPAR))
            {

                if(findSymbol(&symbols,tkName->text))
                    tkerr(crtTk,"symbol redefinition: %s",tkName->text);
                crtFunc=addSymbol(&symbols,tkName->text,CLS_FUNC);
                initSymbols(&crtFunc->args);
                crtFunc->type=t;
                crtDepth++;

                declFunc2();
                if(consume(RPAR))
                {
                    {
                        crtDepth--;
                    }
                    if(stmCompound())
                    {

                        deleteSymbolsAfter(&symbols,crtFunc);
                        crtFunc=NULL;

                        return 1;
                    }else tkerr(crtTk,"missing stmCompound in declFunc");
                } else tkerr(crtTk,"missing ) in declFunc");
            }
        }else tkerr(crtTk,"missing ID in declFunc");
    }
    crtTk = initTk;
    return 0 ;
}

int funcArg()
{
    Token *initTk = crtTk;
    printf("funcArg %s \n", names[crtTk->code]);
    Type t;
    Token *tkName = NULL;

    if(typeBase(&t))
    {
        if(consume(ID))
        {
            tkName = consumedTk;
            if(!arrayDecl(&t))
            {
                t.nElements = -1;
            }


            Symbol  *s=addSymbol(&symbols,tkName->text,CLS_VAR);
            s->mem=MEM_ARG;
            s->type=t;
            s=addSymbol(&crtFunc->args,tkName->text,CLS_VAR);
            s->mem=MEM_ARG;
            s->type=t;

            return 1;
        }else tkerr(crtTk,"missing ID in funcArg");
    }
    crtTk = initTk;
    return 0 ;
}

int stm()
{
    Token *initTk = crtTk;
    printf("stm %s \n", names[crtTk->code]);
    RetVal rv, rv1, rv2, rv3;

    if(stmCompound()){return 1;}
    else if(consume(IF))
    {
        if(consume(LPAR))
        {
            if(expr(&rv))
            {
                if(rv.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be logically tested");
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        if(consume(ELSE))
                        {
                            if(stm()){}
                            else tkerr(crtTk,"lipseste instructiunea lui else");
                        }
                        return 1; printf("%s \n",names[crtTk->code]);
                    }else tkerr(crtTk,"lipseste instructiunea de la if");
                }else tkerr(crtTk,"lipseste ) de la if");
            }else tkerr(crtTk,"lipseste conditia de la if");
        }else tkerr(crtTk,"lipseste ( de la if");
    }else if(consume(WHILE))
    {
        if(consume(LPAR))
        {
            if(expr(&rv))
            {
                if(rv.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be logically tested");
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        return 1; printf("%s \n",names[crtTk->code]);
                    }else tkerr(crtTk,"lipseste instructiunea de la while");
                }else tkerr(crtTk,"lipseste ) de la while");
            }else tkerr(crtTk,"lipseste expresia de la while");
        }else tkerr(crtTk,"lipseste ( de la while");
    }else if(consume(FOR))
    {
        if(consume(LPAR))
        {
            expr(&rv1);
            if(consume(SEMICOLON))
            {
                if(expr(&rv2))
                {
                    if(rv2.type.typeBase==TB_STRUCT)
                        tkerr(crtTk,"a structure cannot be logically tested");
                }
                if(consume(SEMICOLON))
                {
                    expr(&rv3);
                    if(consume(RPAR))
                    {
                        if(stm())
                        {
                            return 1;
                        }else tkerr(crtTk,"lipseste statement de la for");
                    }else tkerr(crtTk,"lipseste ) la for ");
                }else tkerr(crtTk,"lipseste ;2 la for");
            }else {printf("************************ %s \n",names[crtTk->code]);tkerr(crtTk,"lipseste ;1 la for");}
        }else tkerr(crtTk,"lipseste ( la for");
    }else if(consume(BREAK))
    {
        if(consume(SEMICOLON))
        {
            return 1;
        }else tkerr(crtTk,"lipseste ; dupa break");
    }else if(consume(RETURN))
    {
        expr(&rv);
        {
            if(crtFunc->type.typeBase==TB_VOID)
                tkerr(crtTk,"a void function cannot return a value");
            cast(&crtFunc->type,&rv.type);
        }
        if(consume(SEMICOLON))
        {
            return 1;
        }else tkerr(crtTk,"lipseste ; la return ");
    }else
    {
        if(expr(&rv)) {
            if (consume(SEMICOLON)) {
                return 1;

            } else {
               // printf("&&&&&& %s ", names[crtTk->code]);
                tkerr(crtTk, "lipseste ; la finalul stm");
            }
        }else{
            if(consume(SEMICOLON)){
            return 1;}
        }
    }

    crtTk = initTk;
    return 0;
}

int stmCompound()
{
    Token *initTk = crtTk;
    printf("stmCompound %s \n", names[crtTk->code]);
    Symbol *start=symbols.end[-1];


    if(consume(LACC))
    {
        crtDepth++;

        while(1)
        {
            if(declVar())
            {
                continue;
            }
            if(stm()){
                continue;
            }else break;
        }
        if(consume(RACC))
        {
            {
                crtDepth--;
                deleteSymbolsAfter(&symbols,start);
            }
            return 1;
        }else tkerr(crtTk,"lipseste } la stmCompound");

    }
    crtTk = initTk;
    return 0;
}

int expr(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("expr %s \n", names[crtTk->code]);

    if(exprAssign(rv))
    {
        return 1;
    }
    crtTk = initTk;
    return 0;
}

int exprAssign(RetVal *rv)
{
    Token *initTk = crtTk ;
    printf("exprAssign %s \n", names[crtTk->code]);
    RetVal rve;

    if(exprUnary(rv))
    {
        if(consume(ASSIGN))
        {
            if(exprAssign(&rve))
            {
                if (!rv->isLVal)tkerr(crtTk, "cannot assign to a non-lval");
                if (rv->type.nElements>-1 || rve.type.nElements>-1)
                    tkerr(crtTk, "the arrays cannot be assigned");
                cast(&rv->type, &rve.type);
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            }else tkerr(crtTk,"lipseste exprAssign la exprAssign");
        }
    }
    crtTk = initTk;
    if(exprOr(rv))
    {
        return 1;
    }

    crtTk = initTk;
    return 0;
}

int exprOr(RetVal *rv)
{
    Token *initTk = crtTk ;
    printf("exprOr %s \n", names[crtTk->code]);
    if(exprAnd(rv))
    {
        if(exprOr1(rv))
        {
            return 1; printf("%s \n",names[crtTk->code]);
        }else tkerr(crtTk,"lipseste exprOr1 la exprOr");
    }

    crtTk = initTk;
    return 0;
}

int exprOr1(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprOr1 %s \n", names[crtTk->code]);
    RetVal rve;
    if(consume(OR))
    {
        if(exprAnd(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprOr1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste exprOr1 in exprOr1");
        }else tkerr(crtTk,"lipseste exprAnd in exprOr1");
    }else {return 1; }
}

int exprAnd(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprAnd %s \n", names[crtTk->code]);
    if(exprEq(rv))
    {
        if(exprAnd1(rv))
        {
            return 1;
        }else tkerr(crtTk,"lipseste exprAnd1 la exprAnd");
    }
    crtTk = initTk;
    return 0;
}

int exprAnd1(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprAnd1 %s \n", names[crtTk->code]);
    RetVal rve;
     if(consume(AND))
    {
        if(exprEq(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprAnd1(rv))
            {
                return 1; printf("%s \n",names[crtTk->code]);
            }else tkerr(crtTk,"lipseste exprAnd1 in exprAnd1");
        }else tkerr(crtTk,"lipseste exprEq in exprAnd1");
    }else {return 1; printf("%s \n",names[crtTk->code]);}
}

int exprEq(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprEq %s \n", names[crtTk->code]);
    if(exprRel(rv))
    {
        if(exprEq1(rv))
        {
            return 1;
        }else tkerr(crtTk,"lipseste exprEq1 la exprEq");
    }
    crtTk = initTk;
    return 0;
}

int exprEq1(RetVal *rv)
{
    Token *initTk = crtTk, *tkop;
    printf("exprEq1 %s \n", names[crtTk->code]);
    RetVal rve;
    if(consume(EQUAL) )
    {
        tkop = consumedTk;
        if(exprRel(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprEq1(rv))
            {
                return 1;
            }tkerr(crtTk,"lipseste exprEq1 la exprEq1");
        }tkerr(crtTk,"lipseste exprRel la exprEq1");
    }else if(consume(NOTEQ))
    {
        tkop = consumedTk;
        if(exprRel(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprEq1(rv))
            {
                return 1;
            }tkerr(crtTk,"lipseste exprEq1 la exprEq1");
        }tkerr(crtTk,"lipseste exprRel la exprEq1");
    }else{
        return 1;
    }
}

int exprRel(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprRel %s \n", names[crtTk->code]);
    if(exprAdd(rv))
    {
        if(exprRel1(rv))
        {
            return 1; printf("%s \n",names[crtTk->code]);
        }else tkerr(crtTk,"lipseste exprRel1 la exprRel");
    }
    crtTk = initTk;
    return 0 ;
}

int exprRel1(RetVal *rv)
{
    Token *initTk = crtTk, *tkop;
    printf("exprRel1 %s \n", names[crtTk->code]);
    RetVal rve;
    if(consume(LESS))
    {
        tkop = consumedTk;
        if(exprAdd(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprRel1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste eprRel1 la exprRel1");
        }else tkerr(crtTk,"lipseste exprAdd la exprRel1");
    }else if(consume(LESSEQ))
    {
        tkop = consumedTk;
        if(exprAdd(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprRel1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste eprRel1 la exprRel1");
        }else tkerr(crtTk,"lipseste exprAdd la exprRel1");
    }else if(consume(GREATER))
    {
        tkop = consumedTk;
        if(exprAdd(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprRel1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste eprRel1 la exprRel1");
        }else tkerr(crtTk,"lipseste exprAdd la exprRel1");
    }else if(consume(GREATEREQ))
    {
        tkop = consumedTk;
        if(exprAdd(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if(exprRel1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste eprRel1 la exprRel1");
        }else tkerr(crtTk,"lipseste exprAdd la exprRel1");
    }else return 1; printf("%s \n",names[crtTk->code]);
}

int exprAdd(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprAdd %s \n", names[crtTk->code]);
    if(exprMul(rv))
    {
        if(exprAdd1(rv))
        {
            return 1;
        }else tkerr(crtTk,"lipseste exprAdd1 l exprAdd");
    }
    crtTk = initTk;
    return 0;
}

int exprAdd1(RetVal *rv)
{
    Token *initTk = crtTk , *tkop;
    printf("exprAdd1 %s \n", names[crtTk->code]);
    RetVal rve;
    if(consume(ADD))
    {
        tkop = consumedTk;
        if(exprMul(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be added or subtracted");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be added or subtracted");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if(exprAdd1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste exprAdd1 la exprAdd1");
        }else tkerr(crtTk,"lipseste exprMul la exprAdd1");
    }else if(consume(SUB))
    {
        tkop = consumedTk;
        if(exprMul(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be added or subtracted");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be added or subtracted");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if(exprAdd1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste exprAdd1 la exprAdd1");
        }else tkerr(crtTk,"lipseste exprMul la exprAdd1");
    }else return 1; printf("%s \n",names[crtTk->code]);

}

int exprMul(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprMul %s \n", names[crtTk->code]);
    if(exprCast(rv))
    {
        if(exprMul1(rv))
        {
            return 1; printf("%s \n",names[crtTk->code]);
        }else tkerr(crtTk,"lipseste exprMul1 la exprMul");
    }
    crtTk = initTk;
    return 0;
}

int exprMul1(RetVal *rv)
{
    Token *initTk = crtTk, *tkop;
    printf("exprMul1 %s \n", names[crtTk->code]);
    RetVal rve;
    if(consume(MUL))
    {
        tkop = consumedTk;
        if(exprCast(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be multiplied or divided");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be multiplied or divided");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if(exprMul1(rv))
            {
                return 1; printf("%s \n",names[crtTk->code]);
            }else tkerr(crtTk,"lipseste exprMul1 la exprMul1");
        }else tkerr(crtTk,"lipseste exprCast la exprMul1");
    }else if(consume(DIV))
    {
        tkop = consumedTk;
        if(exprCast(&rve))
        {
            if (rv->type.nElements>-1 || rve.type.nElements>-1)
                tkerr(crtTk, "an array cannot be multiplied or divided");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be multiplied or divided");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if(exprMul1(rv))
            {
                return 1; printf("%s \n",names[crtTk->code]);
            }else tkerr(crtTk,"lipseste exprMul1 la exprMul1");
        }else tkerr(crtTk,"lipseste exprCast la exprMul1");
    }else return 1; printf("%s \n",names[crtTk->code]);

}

int exprCast(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprCast %s \n", names[crtTk->code]);
    Type t;
    RetVal rve;
    if(consume(LPAR))
    {
        if(typeName(&t))
        {
            if(consume(RPAR))
            {
                if(exprCast(&rve))
                {
                    cast(&t, &rve.type);
                    rv->type = t;
                    rv->isCtVal = rv->isLVal = 0;
                    return 1; printf("%s \n",names[crtTk->code]);
                }else tkerr(crtTk,"lipseste exprCast la exprCast");
            }else tkerr(crtTk,"lipseste RPAR la exprCast");
        }else tkerr(crtTk,"lipseste typeName la exprCast");
    }else if(exprUnary(rv))
    {
        return 1; printf("%s \n",names[crtTk->code]);
    }

    crtTk = initTk;
    return 0;
}

int exprUnary(RetVal *rv)
{
    Token *initTk = crtTk, *tkop;
    printf("exprUnary %s \n", names[crtTk->code]);
    if(consume(SUB))
    {
        tkop = consumedTk;
        if(exprUnary(rv))
        {
            if (tkop->code == SUB)
            {
                if (rv->type.nElements >= 0)
                    tkerr(crtTk, "unary '-' cannot be applied to an array");
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "unary '-' cannot be applied to a struct");
            }
            else
            {  // NOT
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "'!' cannot be applied to a struct");
                rv->type = createType(TB_INT, -1);
            }
            rv->isCtVal = rv->isLVal = 0;
            return 1;
        }else tkerr(crtTk,"lipseste exprUnary la exprUnary");
    }else if(consume(NOT))
    {
        tkop = consumedTk;
        if(exprUnary(rv))
        {
            if (tkop->code == SUB)
            {
                if (rv->type.nElements >= 0)
                    tkerr(crtTk, "unary '-' cannot be applied to an array");
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "unary '-' cannot be applied to a struct");
            }
            else
            {  // NOT
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "'!' cannot be applied to a struct");
                rv->type = createType(TB_INT, -1);
            }
            rv->isCtVal = rv->isLVal = 0;
            return 1;
        }else tkerr(crtTk,"lipseste exprUnary la exprUnary");
    }else if(exprPostfix(rv))
    {
        return 1;
    }

    crtTk = initTk;
    return 0;
}

int exprPostfix(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprPostfix %s \n", names[crtTk->code]);
    if(exprPrimary(rv))
    {
        if(exprPostfix1(rv))
        {
            return 1;
        }else tkerr(crtTk,"lipseste exprPostfix1 la exprPostfix");
    }

    crtTk = initTk;
    return 0;
}

int exprPostfix1(RetVal *rv)
{
    Token *initTk = crtTk;
    printf("exprPostfix1 %s \n", names[crtTk->code]);
    RetVal rve;
    Token *tkName;
    if(consume(LBRACKET))
    {
        if(expr(&rve))
        {
            if (rv->type.nElements<0)
                tkerr(crtTk, "only an array can be indexed");

            Type typeInt = createType(TB_INT, -1);
            cast(&typeInt, &rve.type);
            rv->type = rv->type;
            rv->type.nElements = -1;
            rv->isLVal = 1;
            rv->isCtVal = 0;
            if(consume(RBRACKET))
            {
                if(exprPostfix1(rv))
                {
                    return 1;
                }else tkerr(crtTk,"lipseste exprPostfix1 la exprPostfix1");
            }else tkerr(crtTk,"lipseste RBRACKET la exprPostfix1");
        }else tkerr(crtTk,"lipseste expr la exprPostfix1");
    }else if(consume(DOT))
    {
        if(consume(ID))
        {
            tkName = consumedTk;
            Symbol      *sStruct = rv->type.s;
            Symbol      *sMember = findSymbol(&sStruct->members, tkName->text);
            if (!sMember)
                tkerr(crtTk, "struct %s does not have a member %s", sStruct->name, tkName->text);
            rv->type = sMember->type;
            rv->isLVal = 1;
            rv->isCtVal = 0;
            if(exprPostfix1(rv))
            {
                return 1;
            }else tkerr(crtTk,"lipseste exprPostfix1 la exprPostfix1");
        }else tkerr(crtTk,"lipseste ID la exprPostfix1");
    }else return 1; printf("%s \n",names[crtTk->code]);
}
/*
int exprPrimary()
{
    Token *initTk = crtTk;
    printf("exprPrimary %s \n", names[crtTk->code]);
    if(consume(ID))
    {
        exprPrimary1();
        return 1;
    }else if(consume(CT_INT))
    {
        return 1;
    }else if(consume(CT_REAL))
    {
        return 1;
    }else if(consume(CT_CHAR))
    {
        return 1;
    }else if(consume(CT_STRING))
    {
        return 1;
    }else if(consume(LPAR))
    {
        if(expr())
        {
            if(consume(RPAR))
            {
                return 1;
            }else tkerr(crtTk,"lipseste RPAR la exprPrimary");
        }else tkerr(crtTk,"lipseste expr la exprPrimary");
    }

    crtTk = initTk;
    return 0;
}

int exprPrimary1()
{
    printf("Primary1 %s \n", names[crtTk->code]);
    if(consume(LPAR))
    {
        exprPrimary2();
        if(consume(RPAR))
        {
            return 1; printf("%s \n",names[crtTk->code]);
        }else tkerr(crtTk,"lipseste RPAR la exprPrimary1");
    }
}

int exprPrimary2()
{
    printf("exprPrimary2 %s \n", names[crtTk->code]);
    if(expr())
    {
        while(1)
        {
            if(consume(COMMA))
            {
                if(expr())
                {

                }
                else {
                    tkerr(crtTk,"lipseste expr dupa COMMA la exprPrimary2");
                    break;
                }
            }else break;
        }
        return 1; printf("%s \n",names[crtTk->code]);
    }
}

*/

  int exprPrimary(RetVal *rv)
  {
      Token *initTk = crtTk;
      Token *tkName;
      Token *tki, *tkr, *tkc, *tks;
      RetVal *arg = (RetVal*)malloc(sizeof(RetVal));

      if (consume(CT_INT))
      {
          tki = consumedTk;
          rv->type = createType(TB_INT, -1); rv->ctVal.i = tki->i;
          rv->isCtVal = 1; rv->isLVal = 0;

          return 1;
      }

      if (consume(CT_REAL))
      {
          tkr = consumedTk;
          rv->type = createType(TB_DOUBLE, -1); rv->ctVal.d = tkr->r;
          rv->isCtVal = 1; rv->isLVal = 0;

          return 1;
      }

      if (consume(CT_CHAR))
      {
          tkc = consumedTk;
          rv->type = createType(TB_CHAR, -1); rv->ctVal.i = tkc->i;
          rv->isCtVal = 1; rv->isLVal = 0;

          return 1;
      }

      if (consume(CT_STRING))
      {
          tks = consumedTk;
          rv->type = createType(TB_CHAR, 0); rv->ctVal.str = tks->text;
          rv->isCtVal = 1; rv->isLVal = 0;

          return 1;
      }

      if (consume(LPAR))
      {
          if (expr(rv))
          {
              if (consume(RPAR))
              {
                  return 1;
              }
              else tkerr(crtTk, " ) missing (exprPrimary)\n");
          }
      }

      if (consume(ID))
      {
          tkName = consumedTk;
          Symbol *s = findSymbol(&symbols, tkName->text);
          if (!s)
              tkerr(crtTk, "undefined symbol %s", tkName->text);
          rv->type = s->type;
          rv->isCtVal = 0;
          rv->isLVal = 1;

          if (consume(LPAR))
          {
              Symbol **crtDefArg = s->args.begin;
              if (s->cls != CLS_FUNC && s->cls != CLS_EXTFUNC)
                  tkerr(crtTk, "call of the non-function %s", tkName->text);

              if (expr(arg))
              {
                  if (crtDefArg == s->args.end)
                      tkerr(crtTk, "too many arguments in call");
                  cast(&(*crtDefArg)->type, &arg->type);
                  crtDefArg++;

                  while (1)
                  {
                      if (consume(COMMA))
                      {
                          if (expr(arg))
                          {
                              if (crtDefArg == s->args.end)
                                  tkerr(crtTk, "too many arguments in call");
                              cast(&(*crtDefArg)->type, &arg->type);
                              crtDefArg++;
                          }
                          else break;
                      }
                      else break;
                  }
              }
              if (consume(RPAR))
              {
                  if (crtDefArg != s->args.end)
                      tkerr(crtTk, "too few arguments in call");
                  rv->type = s->type;
                  rv->isCtVal = rv->isLVal = 0;

                  return 1;
              }
              else tkerr(crtTk, " ) missing (exprPrimary)\n");
          }

          if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
              tkerr(crtTk, "missing call for function %s", tkName->text);

          return 1;
      }

      crtTk = initTk;

      return 0;
  }

//------------------------------------------------------sintactic-----------------------------------------------------------------------------
  void put_s(char s[]);
  void get_s(char s[]);
  void put_i(int i);
  int get_i();
  void put_d(double d);
  double get_d();
  void put_c(char c);
  char get_c();
  double seconds();


  Symbol *addExtFunc(const char *name, Type type)
  {
      Symbol *s = addSymbol(&symbols, name, CLS_EXTFUNC);
      s->type = type;
      initSymbols(&s->args);
      return s;
  }
  Symbol *addFuncArg(Symbol *func, const char *name, Type type)
  {
      Symbol *a = addSymbol(&func->args, name, CLS_VAR);
      a->type = type;
      return a;
  }

  void addExtFuncs()
  {
      Symbol *s;

      s = addExtFunc("put_s", createType(TB_VOID, -1));
      addFuncArg(s, "s", createType(TB_CHAR, 0));

      s = addExtFunc("get_s", createType(TB_VOID, -1));
      addFuncArg(s, "s", createType(TB_CHAR, 0));

      s = addExtFunc("put_i", createType(TB_VOID, -1));
      addFuncArg(s, "s", createType(TB_INT, -1));

      s = addExtFunc("get_i", createType(TB_INT, -1));

      s = addExtFunc("put_d", createType(TB_VOID, -1));
      addFuncArg(s, "s", createType(TB_DOUBLE, -1));

      s = addExtFunc("get_d", createType(TB_DOUBLE, -1));

      s = addExtFunc("put_c", createType(TB_VOID, -1));
      addFuncArg(s, "s", createType(TB_CHAR, 0));

      s = addExtFunc("get_c", createType(TB_CHAR, -1));

      s = addExtFunc("seconds", createType(TB_DOUBLE, -1));
  }


  int main(){
    FILE *f;
    //printf("aici\n");
    f = fopen("C:\\Users\\toshiba\\CLionProjects\\compilator\\9.c","r");
	  //printf("aici\n");
    g= fopen("out.txt","w");
	  //printf("aici\n");
    if(f==NULL){
        printf("err fis \n");
        return -1;
    }
	  //printf("aici\n");

    int i=0;
    char c;
    do{
        c = getc(f);
        buffer[i] = c;
        i++;
    }while(c!=EOF);

    buffer[i-1]='\0';

    pch = buffer;
    line = 1;
    while(getNextToken()!=END){
    }
    //showAtoms();

    crtTk = tokens;
    addExtFuncs();
    if(unit() > 0)
    {
        printf("corect sintactic \n");
    }

    fclose(f);
    fclose(g);

    return 0;

}
