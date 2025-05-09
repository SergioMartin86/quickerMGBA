#include <stdlib.h>
#include <stdint.h>
#include "mgba/core/core.h"
#include "mgba/core/log.h"
#include "mgba/core/timing.h"
#include "mgba/core/serialize.h"
#include "mgba/core/blip_buf.h"
#include "mgba/gba/core.h"
#include "mgba/gba/interface.h"
#include "mgba/internal/gba/gba.h"
#include "mgba/internal/gba/video.h"
#include "mgba/internal/gba/overrides.h"
#include "mgba/internal/arm/isa-inlines.h"
#include "mgba/debugger/debugger.h"
#include "mgba-util/common.h"
#include "mgba-util/vfs.h"

// interop sanity checks
static_assert(sizeof(bool) == 1, "Wrong bool size!");
static_assert(sizeof(color_t) == 2, "Wrong color_t size!");
static_assert(sizeof(enum mWatchpointType) == 4, "Wrong mWatchpointType size!");
static_assert(sizeof(enum GBASavedataType) == 4, "Wrong GBASavedataType size!");
static_assert(sizeof(enum GBAHardwareDevice) == 4, "Wrong GBAHardwareDevice size!");
static_assert(sizeof(ssize_t) == 8, "Wrong ssize_t size!");
static_assert(sizeof(void*) == 8, "Wrong pointer size!");

const char* const binaryName = "mgba";
const char* const projectName = "mGBA BizHawk";
const char* const projectVersion = "(unknown)";

#ifdef _WIN32
#define EXP __declspec(dllexport)
#else
#define EXP __attribute__((visibility("default")))
#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:		the pointer to the member.
 * @type:	   the type of the container struct this is embedded in.
 * @member:	 the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({					  \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})
struct VFile* VFileOpenFD(const char* path, int flags) { return NULL; }

typedef struct
{
	struct mCore* core;
	struct mLogger logger;
	struct GBA* gba; // anything that uses this will be deprecated eventually
	color_t vbuff[GBA_VIDEO_HORIZONTAL_PIXELS * GBA_VIDEO_VERTICAL_PIXELS];
	void* rom;
	struct VFile* romvf;
	uint8_t bios[0x4000];
	struct VFile* biosvf;
	uint8_t sram[0x20000 + 16];
	struct VFile* sramvf;
	struct mKeyCallback keysource;
	struct mRotationSource rotsource;
	struct mRumble rumble;
	struct mRTCSource rtcsource;
	struct GBALuminanceSource lumasource;
	struct mDebugger debugger;
	struct mDebuggerModule module;
	bool attached;
	struct GBACartridgeOverride override;
	int16_t tiltx;
	int16_t tilty;
	int16_t tiltz;
	int64_t time;
	uint8_t light;
	uint16_t keys;
	bool lagged;
	bool skipbios;
	uint32_t palette[0x10000];
} bizctx;

typedef struct
{
	enum GBASavedataType savetype;
	enum GBAHardwareDevice hardware;
	uint32_t idleLoop;
	bool vbaBugCompat;
	bool detectPokemonRomHacks;
} overrideinfo;

struct MemoryAreas 
{
	const void* bios;
	const void* wram;
	const void* iwram;
	const void* mmio;
	const void* palram;
	const void* vram;
	const void* oam;
	const void* rom;
	const void* sram;
};

struct MemorySizes
{
	size_t bios;
	size_t wram;
	size_t iwram;
	size_t mmio;
	size_t palram;
	size_t vram;
	size_t oam;
	size_t rom;
	size_t sram;
};

static int32_t GetX(struct mRotationSource* rotationSource)
{
	return container_of(rotationSource, bizctx, rotsource)->tiltx << 16;
}
static int32_t GetY(struct mRotationSource* rotationSource)
{
	return container_of(rotationSource, bizctx, rotsource)->tilty << 16;
}
static int32_t GetZ(struct mRotationSource* rotationSource)
{
	return container_of(rotationSource, bizctx, rotsource)->tiltz << 16;
}
static uint8_t GetLight(struct GBALuminanceSource* luminanceSource)
{
	return container_of(luminanceSource, bizctx, lumasource)->light;
}
static time_t GetTime(struct mRTCSource* rtcSource)
{
	return container_of(rtcSource, bizctx, rtcsource)->time;
}
static uint16_t GetKeys(struct mKeyCallback* keypadSource)
{
	bizctx *ctx = container_of(keypadSource, bizctx, keysource);
	return ctx->keys;
}
static void RotationCB(struct mRotationSource* rotationSource)
{
}
static void SetRumble(struct mRumble* rumble, int enable)
{
}
static void LightCB(struct GBALuminanceSource* luminanceSource)
{
}
static void TimeCB(struct mRTCSource* rtcSource)
{
	// no, reading the rtc registers should not unset the lagged flag
	// bizctx* ctx = container_of(rtcSource, bizctx, rtcsource);
	// ctx->lagged = false;
}
static void logdebug(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args)
{

}

static void resetinternal(bizctx* ctx)
{
	// RTC will be reinit on GBAOverrideApply
	// The RTC data contents in the sramvf are also read back
	// This is done so RTC data is up to date
	// note reset internally calls GBAOverrideApply potentially, so we do this now
	GBASavedataRTCWrite(&ctx->gba->memory.savedata);

	ctx->core->reset(ctx->core);
	if (ctx->skipbios)
		GBASkipBIOS(ctx->gba);

	if (ctx->gba->memory.rom)
	{
		GBASavedataRTCWrite(&ctx->gba->memory.savedata); // again to be safe (not sure if needed?)
		GBAOverrideApply(ctx->gba, &ctx->override);
	}
}

EXP void BizDestroy(bizctx* ctx)
{
	if (ctx->attached)
	{
		ctx->core->detachDebugger(ctx->core);
		mDebuggerDetachModule(&ctx->debugger, &ctx->module);
		mDebuggerDeinit(&ctx->debugger);
	}

	ctx->core->deinit(ctx->core);
	free(ctx->rom);
	free(ctx);
}

void exec_hook(struct mDebuggerModule* module)
{
}


static void watchpoint_entry(struct mDebuggerModule* module, enum mDebuggerEntryReason reason, struct mDebuggerEntryInfo* info)
{
	module->isPaused = false;
}

EXP ssize_t BizSetWatchpoint(bizctx* ctx, uint32_t addr, enum mWatchpointType type)
{
	struct mWatchpoint watchpoint = {
		.minAddress = addr,
		.maxAddress = addr + 1,
		.segment = -1,
		.type = type
	};
	return ctx->debugger.platform->setWatchpoint(ctx->debugger.platform, &ctx->module, &watchpoint);
}

EXP bool BizClearWatchpoint(bizctx* ctx, ssize_t id)
{
	return ctx->debugger.platform->clearBreakpoint(ctx->debugger.platform, id);
}

EXP void BizSetSoundMask(bizctx* ctx, uint32_t mask)
{
	for (int i = 0; i < 6; i++)
		ctx->core->enableAudioChannel(ctx->core, i, mask & 1 << i);
}

EXP void BizSetLayerMask(bizctx *ctx, uint32_t mask)
{
	for (int i = 0; i < 5; i++)
		ctx->core->enableVideoLayer(ctx->core, i, mask & 1 << i);
}

EXP void* BizCreate(const void* bios, const void* data, uint32_t length, bool skipbios)
{
		overrideinfo oinfo;
		oinfo.savetype = -1;
		oinfo.hardware = 0;
		oinfo.idleLoop = 0;
		oinfo.vbaBugCompat = false;
		oinfo.detectPokemonRomHacks = true;

	bizctx* ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
	{
		return NULL;
	}

	ctx->rom = malloc(length);
	if (!ctx->rom)
	{
		free(ctx);
		return NULL;
	}

 // Setting default palette
	size_t idx = 0;
	for (uint8_t r = 0; r < 16; r++)
	 for (uint8_t g = 0; g < 16; g++)
		 for (uint8_t b = 0; b < 16; b++)
			 for (uint8_t a = 0; a < 16; a++)
				 ctx->palette[idx++] = (16 * r << 24) + (16 * g << 16) + (16 * b << 8) + 16 * a;


	ctx->logger.log = logdebug;
	mLogSetDefaultLogger(&ctx->logger);
	ctx->skipbios = skipbios;

	memcpy(ctx->rom, data, length);
	ctx->romvf = VFileFromMemory(ctx->rom, length);

	if (!ctx->romvf)
	{
		free(ctx->rom);
		free(ctx);
		return NULL;
	}

	ctx->core = GBACoreCreate();
	if (!ctx->core)
	{
		ctx->romvf->close(ctx->romvf);
		free(ctx->rom);
		free(ctx);
		return NULL;
	}

	mCoreInitConfig(ctx->core, NULL);

	if (!ctx->core->init(ctx->core))
	{
		BizDestroy(ctx);
		return NULL;
	}

	ctx->gba = ctx->core->board;

	ctx->core->setVideoBuffer(ctx->core, ctx->vbuff, GBA_VIDEO_HORIZONTAL_PIXELS);
	ctx->core->setAudioBufferSize(ctx->core, 1024);

	blip_set_rates(ctx->core->getAudioChannel(ctx->core, 0), ctx->core->frequency(ctx->core), 44100);
	blip_set_rates(ctx->core->getAudioChannel(ctx->core, 1), ctx->core->frequency(ctx->core), 44100);

	if (!ctx->core->loadROM(ctx->core, ctx->romvf))
	{
		BizDestroy(ctx);
		return NULL;
	}

	memset(ctx->sram, 0xff, sizeof(ctx->sram));
	ctx->sramvf = VFileFromMemory(ctx->sram, sizeof(ctx->sram));
	if (!ctx->sramvf)
	{
		BizDestroy(ctx);
		return NULL;
	}

	ctx->core->loadSave(ctx->core, ctx->sramvf);

	mCoreSetRTC(ctx->core, &ctx->rtcsource);

	ctx->gba->idleOptimization = IDLE_LOOP_IGNORE; // Don't do "idle skipping"
	ctx->gba->keyCallback = &ctx->keysource; // Callback for key reading

	ctx->keysource.readKeys = GetKeys;
	ctx->rotsource.sample = RotationCB;
	ctx->rotsource.readTiltX = GetX;
	ctx->rotsource.readTiltY = GetY;
	ctx->rotsource.readGyroZ = GetZ;
	ctx->rumble.setRumble = SetRumble;
	ctx->lumasource.sample = LightCB;
	ctx->lumasource.readLuminance = GetLight;
	ctx->rtcsource.sample = TimeCB;
	ctx->rtcsource.unixTime = GetTime;
	
	ctx->core->setPeripheral(ctx->core, mPERIPH_ROTATION, &ctx->rotsource);
	ctx->core->setPeripheral(ctx->core, mPERIPH_RUMBLE, &ctx->rumble);
	ctx->core->setPeripheral(ctx->core, mPERIPH_GBA_LUMINANCE, &ctx->lumasource);

	if (bios)
	{
		memcpy(ctx->bios, bios, sizeof(ctx->bios));
		ctx->biosvf = VFileFromMemory(ctx->bios, sizeof(ctx->bios));
		/*if (!GBAIsBIOS(ctx->biosvf))
		{
			ctx->biosvf->close(ctx->biosvf);
			GBADestroy(&ctx->gba);
			free(ctx);
			return NULL;
		}*/
		ctx->core->loadBIOS(ctx->core, ctx->biosvf, 0);
	}

	// setup overrides (largely copy paste from GBAOverrideApplyDefaults, with minor changes for our wants)
	const struct GBACartridge* cart = (const struct GBACartridge*) ctx->gba->memory.rom;
	if (cart)
	{
		memcpy(ctx->override.id, &cart->id, sizeof(ctx->override.id));
		bool isPokemon = false, isKnownPokemon = false;

		if (oinfo.detectPokemonRomHacks)
		{
			static const uint32_t pokemonTable[] =
			{
				// Emerald
				0x4881F3F8, // BPEJ
				0x8C4D3108, // BPES
				0x1F1C08FB, // BPEE
				0x34C9DF89, // BPED
				0xA3FDCCB1, // BPEF
				0xA0AEC80A, // BPEI

				// FireRed
				0x1A81EEDF, // BPRD
				0x3B2056E9, // BPRJ
				0x5DC668F6, // BPRF
				0x73A72167, // BPRI
				0x84EE4776, // BPRE rev 1
				0x9F08064E, // BPRS
				0xBB640DF7, // BPRJ rev 1
				0xDD88761C, // BPRE

				// Ruby
				0x61641576, // AXVE rev 1
				0xAEAC73E6, // AXVE rev 2
				0xF0815EE7, // AXVE
			};

			isPokemon = isPokemon || !strncmp("pokemon red version", &((const char*) ctx->gba->memory.rom)[0x108], 20);
			isPokemon = isPokemon || !strncmp("pokemon emerald version", &((const char*) ctx->gba->memory.rom)[0x108], 24);
			isPokemon = isPokemon || !strncmp("AXVE", &((const char*) ctx->gba->memory.rom)[0xAC], 4);

			if (isPokemon)
			{
				size_t i;
				for (i = 0; !isKnownPokemon && i < sizeof(pokemonTable) / sizeof(*pokemonTable); ++i)
				{
					isKnownPokemon = ctx->gba->romCrc32 == pokemonTable[i];
				}
			}
		}

		if (isPokemon && !isKnownPokemon)
		{
			ctx->override.savetype = GBA_SAVEDATA_FLASH1M;
			ctx->override.hardware = HW_RTC;
			ctx->override.vbaBugCompat = true;
		}
		else
		{
			GBAOverrideFind(NULL, &ctx->override); // apply defaults
			if (oinfo.savetype != GBA_SAVEDATA_AUTODETECT)
			{
				ctx->override.savetype = oinfo.savetype;
			}
			for (int i = 0; i < 5; i++)
			{
				if (!(oinfo.hardware & (128 << i)))
				{
					if (oinfo.hardware & (1 << i))
					{
						ctx->override.hardware |= (1 << i);
					}
					else
					{
						ctx->override.hardware &= ~(1 << i);
					}
				}
			}
			ctx->override.hardware |= oinfo.hardware & 64; // gb player detect
			ctx->override.vbaBugCompat = oinfo.vbaBugCompat;
		}

		ctx->override.idleLoop = oinfo.idleLoop;
	}

	mDebuggerInit(&ctx->debugger);
	ctx->module.type = DEBUGGER_CUSTOM;
	ctx->module.custom = exec_hook;
	ctx->module.entered = watchpoint_entry;
	mDebuggerAttachModule(&ctx->debugger, &ctx->module);
	mDebuggerAttach(&ctx->debugger, ctx->core);
	ctx->attached = true;

	resetinternal(ctx);

	// proper init RTC, our buffer would have trashed it
	if (ctx->override.hardware & HW_RTC)
	{
		GBAHardwareInitRTC(&ctx->gba->memory.hw);
	}

	return ctx;
}

EXP void BizReset(bizctx* ctx)
{
	resetinternal(ctx);
}

void blit(uint32_t* dst, const color_t* src, const uint32_t* palette)
{
	uint32_t* dst_end = dst + GBA_VIDEO_HORIZONTAL_PIXELS * GBA_VIDEO_VERTICAL_PIXELS;

	while (dst < dst_end)
	{
		*dst++ = palette[*src++];
	}
}

EXP bool BizAdvance(bizctx* ctx, uint16_t keys, int64_t time, int16_t gyrox, int16_t gyroy, int16_t gyroz, uint8_t luma, uint32_t* videoBuffer)
{
	ctx->core->setKeys(ctx->core, keys);
	ctx->keys = keys;
	ctx->light = luma;
	ctx->time = time;
	ctx->tiltx = gyrox;
	ctx->tilty = gyroy;
	ctx->tiltz = gyroz;
	ctx->lagged = true;

	ctx->debugger.state = ctx->module.needsCallback ? DEBUGGER_CALLBACK : DEBUGGER_RUNNING;
	mDebuggerRunFrame(&ctx->debugger);

 // Get video 
	if (videoBuffer != NULL) blit(videoBuffer, ctx->vbuff, ctx->palette);

	// *nsamp = blip_samples_avail(ctx->core->getAudioChannel(ctx->core, 0));
	// if (*nsamp > 1024)
	// 	*nsamp = 1024;
	// blip_read_samples(ctx->core->getAudioChannel(ctx->core, 0), sbuff, 1024, true);
	// blip_read_samples(ctx->core->getAudioChannel(ctx->core, 1), sbuff + 1, 1024, true);

	return ctx->lagged;
}

EXP void BizSetPalette(bizctx* ctx, const uint32_t* palette)
{
	memcpy(ctx->palette, palette, sizeof(ctx->palette));
}

EXP void BizGetMemoryAreas(bizctx* ctx, struct MemoryAreas* dst)
{
	size_t sizeOut;
	dst->bios = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_BIOS, &sizeOut);
	dst->wram = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_EWRAM, &sizeOut);
	dst->iwram = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_IWRAM, &sizeOut);
	dst->mmio = ctx->gba->memory.io;
	dst->palram = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_PALETTE_RAM, &sizeOut);
	dst->vram = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_VRAM, &sizeOut);
	dst->oam = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_OAM, &sizeOut);
	dst->rom = ctx->core->getMemoryBlock(ctx->core, GBA_REGION_ROM0, &sizeOut);
	// Return the buffer that BizHawk hands to mGBA for storing savedata.
	// getMemoryBlock is avoided because mGBA doesn't always know save type at startup,
	// so getMemoryBlock may return nothing until the save type is detected.
	// (returning the buffer directly avoids 0-size and variable-size savedata)
	dst->sram = ctx->sram;
	// If ROM is not present (due to multiboot), send our raw buffer over
	if (!dst->rom)
	{
		dst->rom = ctx->rom;
	}
}

EXP void BizGetMemorySizes(bizctx* ctx, struct MemorySizes* dst)
{
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_BIOS, &dst->bios);
 ctx->core->getMemoryBlock(ctx->core, GBA_REGION_EWRAM, &dst->wram);
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_IWRAM, &dst->iwram);
	dst->mmio = sizeof(ctx->gba->memory.io);
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_PALETTE_RAM, &dst->palram);
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_VRAM, &dst->vram);
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_OAM, &dst->oam);
	ctx->core->getMemoryBlock(ctx->core, GBA_REGION_ROM0, &dst->rom);
	// Return the buffer that BizHawk hands to mGBA for storing savedata.
	// getMemoryBlock is avoided because mGBA doesn't always know save type at startup,
	// so getMemoryBlock may return nothing until the save type is detected.
	// (returning the buffer directly avoids 0-size and variable-size savedata)
	dst->sram = sizeof(ctx->sram);
	// If ROM is not present (due to multiboot), send our raw buffer over
	if (!dst->rom)
	{
		dst->rom = sizeof(ctx->rom);
	}
}

EXP uint32_t BizGetSaveRam(bizctx* ctx, void* data, uint32_t size)
{
	GBASavedataRTCWrite(&ctx->gba->memory.savedata); // make sure RTC data is up to date
	ctx->sramvf->seek(ctx->sramvf, 0, SEEK_SET);
	return ctx->sramvf->read(ctx->sramvf, data, size);
}

EXP void BizPutSaveRam(bizctx* ctx, const void* data, uint32_t size)
{
	ctx->sramvf->seek(ctx->sramvf, 0, SEEK_SET);
	ctx->sramvf->write(ctx->sramvf, data, size);
	if ((ctx->override.hardware & HW_RTC) && (size & 0xff)) // RTC data is in the save, read it out
	{
		GBASavedataRTCRead(&ctx->gba->memory.savedata);
	}
}

// state sizes can vary!
EXP bool BizStartGetState(bizctx* ctx, struct VFile** file, uint32_t* size)
{
	struct VFile* vf = VFileMemChunk(NULL, 0);
	if (!mCoreSaveStateNamed(ctx->core, vf, SAVESTATE_SAVEDATA))
	{
		vf->close(vf);
		return false;
	}
	*file = vf;
	*size = vf->seek(vf, 0, SEEK_END);
	return true;
}

EXP void BizFinishGetState(struct VFile* file, void* data, uint32_t size)
{
	file->seek(file, 0, SEEK_SET);
	file->read(file, data, size);
	file->close(file);
}

EXP bool BizPutState(bizctx* ctx, const void* data, uint32_t size)
{
	struct VFile* vf = VFileFromConstMemory(data, size);
	bool ret = mCoreLoadStateNamed(ctx->core, vf, SAVESTATE_SAVEDATA);
	vf->close(vf);
	return ret;
}

EXP void BizGetRegisters(bizctx* ctx, int32_t* dest)
{
	memcpy(dest, ctx->gba->cpu, 18 * sizeof(int32_t));
}

EXP void BizSetRegister(bizctx* ctx, int32_t index, int32_t value)
{
	if (index >= 0 && index < 16)
	{
		ctx->gba->cpu->gprs[index] = value;	
	}

	if (index == 16)
	{
		memcpy(&ctx->gba->cpu->cpsr, &value, sizeof(int32_t));
	}

	if (index == 17)
	{
		memcpy(&ctx->gba->cpu->spsr, &value, sizeof(int32_t));
	}
}

EXP uint64_t BizGetGlobalTime(bizctx* ctx)
{
	return mTimingGlobalTime(ctx->debugger.core->timing);
}

EXP void BizWriteBus(bizctx* ctx, uint32_t addr, uint8_t val)
{
	ctx->core->rawWrite8(ctx->core, addr, -1, val);
}

EXP uint8_t BizReadBus(bizctx* ctx, uint32_t addr)
{
	return ctx->core->busRead8(ctx->core, addr);
}
