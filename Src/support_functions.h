void initMatrix(void);
void generateRandomMatrix(void);
void getMatrixNextIndexes(int*);
void validateMatrix(void);
void serializeMatrixStr(void);
void loadMatrixFromFile(char*);
void acceptClient(void);
void* clientHandler(void*);
void timerHandler(int);
void setAlarm(void);
void startGame(void);
void loadDictionary(void);
void searchWordInMatrix(int, int);
void restoreWords(void);
void validateDictionary(void);
int validateWord(char*);

