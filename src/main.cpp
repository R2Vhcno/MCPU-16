#include <iostream>
#include <fstream>

#include "include/M16.h"

int main(int argc, const char* argv[]) {
	if (argc > 2 || argc <= 1) {
		printf("Usage: m16 [assembly]");
		getchar();
		exit(64);
	}

	m16::cpu* vm = new m16::cpu();

	m16::micrasm assembly;

	FILE* file;
	fopen_s(&file, argv[1], "rb");

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* src = new char[fileSize + 1];
	size_t bytesRead = fread(src, sizeof(char), fileSize, file);
	src[bytesRead] = '\0';

	fclose(file);

	try {
		assembly.assemble(src);
	} catch (m16::micrasm_error e) {
		printf("[ERROR] - %s\n", e.what());
		return -1;
	}

	delete[] src;

	//vm->loadImage(assembly.getCode());
	vm->loadImage(assembly.getCode());

	vm->dumpMem();
	
	while (!vm->debugHalt) {
		vm->process();
	}

	vm->printRegs();

	delete vm;

	getchar();
	return 0;
}