#include <Windows.h>
#include <DbgHelp.h>
#include "app_config.h"
#include "unity.h"

#pragma comment(lib, "Dbghelp.lib")

int RunApplication(appConfig config, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void Cbuf_ExecuteText(int exec_when, const char *text);
void CL_Init(void);
int checkIfCommandExists(char* command);
int Sys_MilliSeconds(void);
appConfig PlayerConnectsToGame();
appConfig PlayerDisconnectsFromGame();
appConfig PlayerKickedFromGame();
int clientIntialized = 0;
int mapSet = 0;
HANDLE conHandle;
appConfig activeTestConfig;
appConfig breakDownConfig;

void printStack(void)
{
	unsigned int   i;
	void         * stack[100];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;

	process = GetCurrentProcess();

	SymInitialize(process, NULL, TRUE);

	frames = CaptureStackBackTrace(0, 100, stack, NULL);
	symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (i = 0; i < frames; i++)
	{
		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);

		printf("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
	}

	free(symbol);
}

#define MAX_TEST_CASES 10

typedef struct testCase {
	void* testFunction;
	char* testName;
	appConfig testConfig;
} testCase;

testCase testCases[MAX_TEST_CASES];
int testCaseAmount;

void clientConnectivity(void)
{
	RunApplication(activeTestConfig, NULL, NULL, "", SW_SHOW);
}

typedef struct color_
{
	int* white;
	int* gray;
	int* blue;
	int* green;
	int* red;
	int* yellow;
} color_;

color_ Color;

void setColor(int* color)
{
	SetConsoleTextAttribute(conHandle, color);
}

void setupClient()
{
	int start = Sys_MilliSeconds();
	if (!clientIntialized) {
		fputs("Initializing client side...", stdout);
		CL_Init();
		printf("Done in %d ms", (Sys_MilliSeconds() - start));
		puts("");
		clientIntialized = 1;
	}
	else {
		puts("Client already initialized, skipping...");
	}
}

void setupMap(char* map)
{
	int start = Sys_MilliSeconds();
	if (!map) map = "q3dm7";
	if (!mapSet) {
		fputs("Setting server map to q3dm7...", stdout);
		char* m[20];
		sprintf(m, "map %s", map);
		Cbuf_ExecuteText(1, m);
		printf("Done in %d ms", (Sys_MilliSeconds() - start));
		puts("");
		mapSet = 1;
	}
	else {
		puts("Map already set, skipping...");
	}
}

void execConfig(appConfig config, int time)
{
	if (config.execString == "") {
		puts("Config had no execString, skipping...\n");
	}
	else {
		printf("Running execString: ");
		setColor(Color.yellow);
		printf(config.execString);
		setColor(Color.gray);
		Cbuf_ExecuteText(1, config.execString);

		printf("\n - TEST DONE in %d ms", (Sys_MilliSeconds() - time));

		puts("");
		puts("");
	}
}

int setupEnviroment()
{
	int first = Sys_MilliSeconds();
	setupClient();
	setupMap(NULL);
	return first;
}

void threadFunc(appConfig config) {

	if (config.breakDown)
	{
		puts("\nBreaking down quake...\n");
		Cbuf_ExecuteText(1, config.execString);
		
		puts("\n:::Resetting game state...:::\n");
		clientIntialized = 0;
		mapSet = 0;

		return;
	}

	setColor(Color.green);
	printf(config.testName);
	setColor(Color.gray);

	int time = setupEnviroment();
	execConfig(config, time);

	if (config.reset) {
		puts("\n:::Resetting game state...:::\n");
		clientIntialized = 0;
		mapSet = 0;
	}
}

HANDLE startThread(void* func, int* threadId) {
	
	return CreateThread(
		NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		0,		// DWORD cbStack,
		(LPTHREAD_START_ROUTINE)func,	// LPTHREAD_START_ROUTINE lpStartAddr,
		0,			// LPVOID lpvThreadParm,
		0,			//   DWORD fdwCreate,
		&threadId);
}

int equals(char* c1, char* c2) {
	if (!c1 || !c2) return 0;
	int i = 0;
	while (1) {
		if ((c1[i] == '\0' && c2[i] != '\0') ||
			(c1[i] != '\0' && c2[i] == '\0')) {
			return 0;
		}
		if (c1[i] != c2[i]) return 0;
		if (c1[i] == '\0' && c2[i] == '\0') break;
		i++;
	}
	return 1;
}

void printTestCases()
{
	int count = 1;
	for (int i = 0; i < testCaseAmount; i++)
	{
		if (testCases[i].testName != NULL)
		{
			printf("%d - %s\n", i+1, testCases[i].testName);
			count++;
		}
	}
	printf("\n%d - Run all test cases\n", count);
}

void setupColors()
{
	Color.white = 15;
	Color.blue = 1;
	Color.green = 2;
	Color.red = 12;
	Color.gray = 7;
	Color.yellow = 6;
}

void addTestCase(testCase tc)
{
	for (int i = 0; i < MAX_TEST_CASES; i++)
	{
		if (testCases[i].testName == NULL)
		{
			testCaseAmount = i + 1;
			testCases[i] = tc;
			return;
		}
	}
}

void initBreakDownConfig()
{
	breakDownConfig.ptr = &threadFunc;
	breakDownConfig.execString = "quit";
	breakDownConfig.finished = FALSE;
	breakDownConfig.next = 0;
	breakDownConfig.reset = TRUE;
	breakDownConfig.breakDown = TRUE;
}

void setupTests()
{
	// BREAKDOWN CONFIG INIT
	initBreakDownConfig();

	// Arrange 
	testCase testCaseOne;
	testCaseOne.testFunction = &clientConnectivity;

	// Act 1
	testCaseOne.testName = "Client connectivity";
	testCaseOne.testConfig = PlayerConnectsToGame();
	addTestCase(testCaseOne);

	// Act 2
	testCaseOne.testName = "Client disconnects";
	testCaseOne.testConfig = PlayerDisconnectsFromGame();
	addTestCase(testCaseOne);

	// Act 3
	testCaseOne.testName = "Client kicked from game";
	testCaseOne.testConfig = PlayerKickedFromGame();
	addTestCase(testCaseOne);


	// RINSE, REPEAT
}

void runTest(int testIndex) {
	int threadId;
	testCase tc = testCases[testIndex];
	if (tc.testConfig.ptr != 0xcccccccc)
	{
		activeTestConfig = tc.testConfig;
		HANDLE threadHandle = startThread(tc.testFunction, &threadId);
	}
	
}

int main(void) {

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	setupColors();
	setupTests();

	puts("Starting quake3 test environment...");
	puts("");

	char* line[1024];
	printTestCases();
	printf("\nSelect test to run: ");

	do {

		scanf("%s", line);
		system("cls");

		int number = atoi(line);
		if (number <= MAX_TEST_CASES && number > 0)
		{
			number--;

			if (number == testCaseAmount)
			{
				for (int i = 0; i < testCaseAmount; i++)
				{
					if (testCases[i].testName != NULL)
						runTest(i);
				}
			}
			else
			{
				if (testCases[number].testName != NULL)
					runTest(number);
			}
		}

	} while (!equals(line, "exit"));
}

appConfig PlayerConnectsToGame(){

	// Assign
	appConfig config;
	config.testName = "TEST # 1 - Player tries to connect to a online game.\n";
	config.errorType = CONNECT_FAIL;
	config.ptr = &threadFunc;
	config.execString = "connect 127.0.0.1";
	config.finished = FALSE;
	config.reset = FALSE;
	config.server = TRUE;
	config.breakDown = FALSE;

	config.next = &breakDownConfig;

	return config;
}

appConfig PlayerDisconnectsFromGame(){

	// Assign
	appConfig config;
	config.testName = "";
	config.errorType = CONNECT_FAIL;
	config.ptr = &threadFunc;
	config.execString = "connect 127.0.0.1";
	config.finished = FALSE;
	config.reset = FALSE;
	config.server = TRUE;
	config.breakDown = FALSE;

	appConfig config2;
	config2.testName = "TEST # 2 - Player tries to disconnect from a online game.\n";
	config2.errorType = CONNECT_FAIL;
	config2.ptr = &threadFunc;
	config2.execString = "disconnect";
	config2.finished = FALSE;
	config2.prev = &config;
	config2.next = NULL;
	config2.breakDown = FALSE;
	config2.reset = FALSE;

	config.next = &config2;
	
	return config;
}

appConfig PlayerKickedFromGame(){

	// Assign
	appConfig config;
	config.testName = "";
	config.errorType = CONNECT_FAIL;
	config.ptr = &threadFunc;
	config.execString = "connect 127.0.0.1";
	config.finished = FALSE;
	config.reset = FALSE;
	config.server = TRUE;
	config.breakDown = FALSE;

	appConfig config2;
	config2.testName = "TEST # 3 - Connected player gets kicked by host.\n";
	config2.errorType = CONNECT_FAIL;
	config2.ptr = &threadFunc;
	config2.execString = "kick HSKINGEN2";
	config2.finished = FALSE;
	config2.prev = &config2;
	config2.next = NULL;
	config2.breakDown = FALSE;
	config2.reset = TRUE;

	config.next = &config2;

	return config;
}








