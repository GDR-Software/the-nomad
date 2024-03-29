#include "module_public.h"
#include "module_alloc.h"
#include "module_virtual_asm.h"
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>

//OSX has MAP_ANON
#ifndef MAP_ANONYMOUS
	#define MAP_ANONYMOUS MAP_ANON
#endif

namespace assembler {

unsigned Processor::maxIntArgs64() {
	return 6;
}

unsigned Processor::maxFloatArgs64() {
	return 8;
}

bool Processor::isIntArg64Register(unsigned char number, unsigned char arg) {
	return number < 6;
}

bool Processor::isFloatArg64Register(unsigned char number, unsigned char arg) {
	return number < 8;
}

Register Processor::intArg64(unsigned char number, unsigned char arg) {
	switch(number) {
		case 0:
			return Register(*this, EDI);
		case 1:
			return Register(*this, ESI);
		case 2:
			return Register(*this, EDX);
		case 3:
			return Register(*this, ECX);
		case 4:
			return Register(*this, R8);
		case 5:
			return Register(*this, R9);
		default:
			throw "Integer64 argument index out of bounds";
	}
}

Register Processor::floatArg64(unsigned char number, unsigned char arg) {
	switch(number) {
		case 0:
			return Register(*this, XMM0);
		case 1:
			return Register(*this, XMM1);
		case 2:
			return Register(*this, XMM2);
		case 3:
			return Register(*this, XMM3);
		case 4:
			return Register(*this, XMM4);
		case 5:
			return Register(*this, XMM5);
		case 6:
			return Register(*this, XMM6);
		case 7:
			return Register(*this, XMM7);
		default:
			throw "Float64 argument index out of bounds";
	}
}

Register Processor::intArg64(unsigned char number, unsigned char arg, Register defaultReg) {
	if(isIntArg64Register(number, arg))
		return intArg64(number, arg);
	return defaultReg;
}

Register Processor::floatArg64(unsigned char number, unsigned char arg, Register defaultReg) {
	if(isFloatArg64Register(number, arg))
		return floatArg64(number, arg);
	return defaultReg;
}

Register Processor::intReturn64() {
	return Register(*this, EAX);
}

Register Processor::floatReturn64() {
	return Register(*this, XMM0);
}

CodePage::CodePage(unsigned int Size, void* requestedStart) : used(0), final(false), references(1) {
	unsigned minPageSize = getMinimumPageSize();
	unsigned pages = Size / minPageSize;

	if(Size % minPageSize != 0)
		pages += 1;

	size_t reqptr = (size_t)requestedStart;
	if(reqptr % minPageSize != 0)
		reqptr -= (reqptr % minPageSize);

	page = mmap(
		(void*)reqptr,
		Size,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_PRIVATE,
		0,
		0);

	size = pages * minPageSize;
}

void CodePage::grab() {
	++references;
}

void CodePage::drop() {
	if(--references == 0)
		delete this;
}

CodePage::~CodePage() {
	munmap(page, size);
}

void CodePage::finalize() {
	mprotect(page, size, PROT_READ | PROT_EXEC);
	final = true;
}

unsigned int CodePage::getMinimumPageSize() {
	return getpagesize();
}

void CriticalSection::enter() {
    m_Lock.Lock();
}

void CriticalSection::leave() {
    m_Lock.Unlock();
}

CriticalSection::CriticalSection() {
}

CriticalSection::~CriticalSection() {
}

};