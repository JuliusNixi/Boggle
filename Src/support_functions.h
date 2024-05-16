void initMatrix(void);
void generateRandomMatrix(void);
void getMatrixNextIndexes(int*);
void validateMatrix(void);
char* serializeMatrixStr(void);
void loadMatrixFromFile(char*);
void acceptClient(void);
void* clientHandler(void*);
void timerHandler(int);
void setAlarm(void);
void startGame(void);
void loadDictionary(char*);
int searchWordInMatrix(int, int, char*);
void validateDictionary(void);
int validateWord(char*);
void sigintHandler(int);

