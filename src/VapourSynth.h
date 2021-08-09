/*
* Copyright (c) 2012-2017 Fredrik Mellbin
*
* This file is part of VapourSynth.
*
* VapourSynth is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* VapourSynth is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with VapourSynth; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef VAPOURSYNTH_H
#define VAPOURSYNTH_H

#include <stdint.h>

#define VAPOURSYNTH_API_MAJOR 3
#define VAPOURSYNTH_API_MINOR 6
#define VAPOURSYNTH_API_VERSION ((VAPOURSYNTH_API_MAJOR << 16) | (VAPOURSYNTH_API_MINOR))

/* Convenience for C++ users. */
#ifdef __cplusplus
#    define VS_EXTERN_C extern "C"
#    if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
#        define VS_NOEXCEPT noexcept
#    else
#        define VS_NOEXCEPT
#    endif
#else
#    define VS_EXTERN_C
#    define VS_NOEXCEPT
#endif

#if defined(_WIN32) && !defined(_WIN64)
#    define VS_CC __stdcall
#else
#    define VS_CC
#endif

/* And now for some symbol hide-and-seek... */
#if defined(_WIN32) /* Windows being special */
#    define VS_EXTERNAL_API(ret) VS_EXTERN_C __declspec(dllexport) ret VS_CC
#elif defined(__GNUC__) && __GNUC__ >= 4
#    define VS_EXTERNAL_API(ret) VS_EXTERN_C __attribute__((visibility("default"))) ret VS_CC
#else
#    define VS_EXTERNAL_API(ret) VS_EXTERN_C ret VS_CC
#endif

#if !defined(VS_CORE_EXPORTS) && defined(_WIN32)
#    define VS_API(ret) VS_EXTERN_C __declspec(dllimport) ret VS_CC
#else
#    define VS_API(ret) VS_EXTERNAL_API(ret)
#endif

typedef struct VSFrameRef VSFrameRef;
typedef struct VSNodeRef VSNodeRef;
typedef struct VSCore VSCore;
typedef struct VSPlugin VSPlugin;
typedef struct VSNode VSNode;
typedef struct VSFuncRef VSFuncRef;
/* VSMap note:
All values beginning with '_' are considered to be system defined. These
are checked and setting them to undefined vallues will lead to a fatal error. */
typedef struct VSMap VSMap;
typedef struct VSAPI VSAPI;
typedef struct VSFrameContext VSFrameContext;

typedef enum VSColorFamily {
    /* New API V4 constants */
    cfUndefined = 0,
    cfGray      = 1,
    cfRGB       = 2,
    cfYUV       = 3,
    /* (end of new API V4 constants) */
    /* all planar formats */
    cmGray   = 1000000,
    cmRGB    = 2000000,
    cmYUV    = 3000000,
    cmYCoCg  = 4000000,
    /* special for compatibility */
    cmCompat = 9000000
} VSColorFamily;

typedef enum VSSampleType {
    stInteger = 0,
    stFloat = 1
} VSSampleType;

/* The +10 is so people won't be using the constants interchangeably "by accident" */
typedef enum VSPresetFormat {
    pfNone = 0,

    pfGray8 = cmGray + 10,
    pfGray16,

    pfGrayH,
    pfGrayS,

    pfYUV420P8 = cmYUV + 10,
    pfYUV422P8,
    pfYUV444P8,
    pfYUV410P8,
    pfYUV411P8,
    pfYUV440P8,

    pfYUV420P9,
    pfYUV422P9,
    pfYUV444P9,

    pfYUV420P10,
    pfYUV422P10,
    pfYUV444P10,

    pfYUV420P16,
    pfYUV422P16,
    pfYUV444P16,

    pfYUV444PH,
    pfYUV444PS,

    pfYUV420P12,
    pfYUV422P12,
    pfYUV444P12,

    pfYUV420P14,
    pfYUV422P14,
    pfYUV444P14,

    pfRGB24 = cmRGB + 10,
    pfRGB27,
    pfRGB30,
    pfRGB48,

    pfRGBH,
    pfRGBS,

    /* special for compatibility, if you implement these in any filter I'll personally kill you */
    /* I'll also change their ids around to break your stuff regularly */
    pfCompatBGR32 = cmCompat + 10,
    pfCompatYUY2
} VSPresetFormat;

typedef enum VSFilterMode {
    fmParallel = 100, /* completely parallel execution */
    fmParallelRequests = 200, /* for filters that are serial in nature but can request one or more frames they need in advance */
    fmUnordered = 300, /* for filters that modify their internal state every request */
    fmSerial = 400 /* for source filters and compatibility with other filtering architectures */
} VSFilterMode;

typedef struct VSFormat {
    char name[32];
    int id;
    int colorFamily; /* see VSColorFamily */
    int sampleType; /* see VSSampleType */
    int bitsPerSample; /* number of significant bits */
    int bytesPerSample; /* actual storage is always in a power of 2 and the smallest possible that can fit the number of bits used per sample */

    int subSamplingW; /* log2 subsampling factor, applied to second and third plane */
    int subSamplingH;

    int numPlanes; /* implicit from colorFamily */
} VSFormat;

typedef enum VSNodeFlags {
    nfNoCache    = 1,
    nfIsCache    = 2,
    nfMakeLinear = 4 /* api 3.3 */
} VSNodeFlags;

typedef enum VSPropTypes {
    ptUnset = 'u',
    ptInt = 'i',
    ptFloat = 'f',
    ptData = 's',
    ptNode = 'c',
    ptFrame = 'v',
    ptFunction = 'm'
} VSPropTypes;

typedef enum VSGetPropErrors {
    peUnset = 1,
    peType  = 2,
    peIndex = 4
} VSGetPropErrors;

typedef enum VSPropAppendMode {
    paReplace = 0,
    paAppend  = 1,
    paTouch   = 2
} VSPropAppendMode;

typedef struct VSCoreInfo {
    const char *versionString;   /* A null-terminated string with full version and legal stuff. Character encoding ??? */
    int core;                    /* Core revision number */
    int api;                     /* Supported API version. */
    int numThreads;              /* Current number of working threads. */
    int64_t maxFramebufferSize;  /* Limit of the frame buffer, in bytes. */
    int64_t usedFramebufferSize; /* Current amount of used frame buffer, in bytes. */
} VSCoreInfo;

/* Important note: width, height and format can be null.
In this case, the zeroed field is considered variable and could change
from one frame to another. You must check the frame properties to
know the actual format. */
typedef struct VSVideoInfo {
    const VSFormat *format;
    int64_t fpsNum;
    int64_t fpsDen;
    int width;
    int height;
    int numFrames; /* api 3.2 - no longer allowed to be 0 */
    int flags;
} VSVideoInfo;

typedef enum VSActivationReason {
    arInitial = 0,
    arFrameReady = 1,
    arAllFramesReady = 2,
    arError = -1
} VSActivationReason;

typedef enum VSMessageType {
    mtDebug = 0,
    mtWarning = 1,
    mtCritical = 2,
    mtFatal = 3
} VSMessageType;



/* core entry point */
typedef const VSAPI *(VS_CC *VSGetVapourSynthAPI)(int version);



/* plugin function and filter typedefs */

/*
==============================================================================
Name: *VSPublicFunction
Description:
	Plug-in function called from invoke() by the host to create a filter
	instance.
	In this function, the plug-in should call createFilter() to create the
	instance node then attach its own data to it. Then it has to populate
	the out map which is returned to the invoke() caller.
	If for some reason you cannot create the filter, you have to free any
	created node reference using freeNode() and call setError() on out before
	exiting.
Input parameters:
	- in: Input parameter list.
		Use coreFuncs->propGet*() to retrieve a parameter value.
		The map is guaranteed to exist until filter destruction, so the plug-in
		can hold references on strings without worry.
	- userData: Pointer that was provided during function registration.
	- core: Pointer on the main host object.
	- vsapi: Pointer on the host function set.
Output parameters:
	- out: Output parameter list. Generally contains a reference on the output
		clip, which key name is "clip", but could just contain some numbers
		or stuff.
		It shouldn't contain any "_Error" key excepted in case of error.
==============================================================================
*/

typedef void (VS_CC *VSPublicFunction)(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);



/*
==============================================================================
Name: registerFunction
Description:
	Host function called by the plug-in in VapourSynthPluginInit().
	Declares a new function.
Input parameters:
	- name: function name. Only 'a'-'z','A'-'Z','0'-'9' allowed, no initial
		number, this applies to most identifiers in Varpoursynth.
		Null-terminated string.
	- args: string containing the list of the arguments.
		Null-terminated string. Arguments are separated by a semicolon (';').
		Each arguments is made of several fields separated by a colon (':').
		Don't insert additional whitespace characters.
			- The argument name. Same character set specifications as the
				function name.
			- The type. Can be:
					"clip" : const VSNodeRef *,
					"int"  : int64_t,
					"float": double,
					"frame": const VSFrameRef *,
					"data" : const char * (a null-terminated string or an opaque
						array of bytes).
				It is possible to declare an array by appending "[]" to the type.
			- "opt" if the parameter is optional.
			- "link" if the parameter can be automated per frame.
				There should be a second property with "_prop" appended at its
				end and whose type is "data".
	- argsFunc: function called by the host to create an instance of the
		filter.
	- functionData: pointer (on user data) passed to argsFunc when creating a
		filter. Useful to declare multiple filters using a single function +
		a parameter.
Input/output parameters:
	- plugin: pointer to the plug-in object in the host, as passed to
		VapourSynthPluginInit().
==============================================================================
*/

typedef void (VS_CC *VSRegisterFunction)(const char *name, const char *args, VSPublicFunction argsFunc, void *functionData, VSPlugin *plugin);



/*
==============================================================================
Name: *VSConfigPlugin
Description:
	Host function, called by the plug-in in VapourSynthPluginInit().
	Cannot be called twice. Mandatory call.
Input parameters:
	- identifier: Unique plug-in identifer. Null-terminated string.
		It has to be globally unique among all filters ever made or death
		ensues, preferably based on a domain you own if you own no domains then
		enter something like com.plugin.shortidentifierhere.
		Example: "com.vapoursynth.std"
	- defaultNamespace: All declared functions go in this namespace.
		Must be unique too or you get an error.
		Example: "std"
	- name: Plug-in name in readable form. Null-terminated string.
		Any string that describes the plugin, something like
		"Blah filter collection version 1.5" is suitable.
	- apiVersion: VAPOURSYNTH_API_VERSION
	- readonly: should always be set to 1 unless you're implementing a plugin-
		compatibility layer or similar. It controls injection of functions into
		other plugins namespaces, hence say just pass 1.
Input/output parameters:
	- plugin: pointer to the plug-in object in the host, as passed to
		VapourSynthPluginInit().
==============================================================================
*/

typedef void (VS_CC *VSConfigPlugin)(const char *identifier, const char *defaultNamespace, const char *name, int apiVersion, int readonly, VSPlugin *plugin);



/*
==============================================================================
Name: *VSInitPlugin
Description:
	Plug-in function, named VapourSynthPluginInit, called by the host
	for initialisation.
	Use extern "C" to declare it in C++ so its name doesn't get mangled.
Input parameters:
	- configFunc: callback for the plug-in
	- registerFunc: callback for the plug-in
Input/output parameters:
	- plugin: the plug-in object in the host.
==============================================================================
*/

typedef void (VS_CC *VSInitPlugin)(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin);



typedef void (VS_CC *VSFreeFuncData)(void *userData);



/*
==============================================================================
Name: *VSFilterInit
Description:
	Plug-in function called by the host during filter instantiation.
	The function hast been passed to the host in the call to createFilter().
	The calls are nested the following way:
		host      invoke()
		plug-in   VSPublicFunction()
		host      createFilter()
		plug-in   VSFilterInit()
	You must set the video information to the node during the call.
	Upon error, call SetError() and return.
Input parameters:
	- core: Pointer on the main host object.
	- vsapi: Pointer on the host function set.
Input/output parameters:
	- in: Pointer on a copy of the input parameter list residing in the node
		object. You can wipe the map out if you don't need the referenced data
		anymore.
	- out: Pointer on the output parameter list, as passed to createFilter().
	- instanceData: A pointer on the pointer on the private filter data, as
		passed by the plug-in to the createFilter() function. It's possible
		here to change the pointed structure.
	- node: This is a pointer on the filter object on the host side, the node
		in the signal flow graph. Plug-in needs to set the video information
		(for the output clip) before the function returns.
==============================================================================
*/

typedef void (VS_CC *VSFilterInit)(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi);



/*
==============================================================================
Name: *VSFilterGetFrame
Description:
	Plug-in function called by the host to generate an output frame.
	The frame generation is done in multiple steps, indicated by
	activationReason. Thus, one can request the input frames during the first
	call, return immediately and be called back later when they are ready, to
	generate the output frame.
	It's possible to allocate local data, persistant during the multiple
	calls requesting the output frame.
	In case of error, call setFilterError() before returning 0 and
	deallocating *frameData if required.
	Depending on the VSFilterMode set for the filter, multiple output frames
	could be requested concurrently.
	There is no concurrent calls for the same frame, if a frame is already
	going new requests simply wait and snatch it too when it's done.
Input parameters:
	- n: Requested frame index. The requested frame is always guaranteed to
		be >= 0, requesting negative frame numbers is considered a fatal error.
		Filters may however request beyond the range of your reported clip
		(this is to facilitate an unknown number of frames without crashing
		too).
	- activationReason: One of the ActivationReason value. The plug-in must
		check the activation reason value before doing anything:
			- arInitial: first call. The plug-in should request the input
				frames now, using requestFrameFilter().
			- arFrameReady: The function is also called when one frame has been
				completed. Most filters will not care about this, it is only if
				you need to inspect maybe 15+ frames and can do incremental
				calculations. The plug-in could immediately use the new data
				with getFrameFilter().
			- arAllFramesReady: Called when the last input frame is ready.
				Now the plug-in has to deliver the output frame.
		frames or emitting an error.
	- core: Pointer on the main host object.
	- vsapi: Pointer on the host function set.
Input/output parameters:
	- instanceData: A double indirection on the private filter data.
		The plug-in can change the data or the pointer.
	- frameData: double indirection on optional private data associated with
		the output frame. The plug-in can allocate data during the first calls
		and store the pointer to *frameData. Data must be deallocated before
		the last call for the given frame (arAllFramesReady or error).
	- frameCtx: A context that the plug-in has to pass when requesting
Returns:
	A reference to the output frame when it is ready, or 0.
	The ownership of the frame is transmitted to the caller.
	You may only return a frame when you have no outstanding frame requests,
	meaning either when called with arInitial and no requests have been done
	or when called with arAllFramesReady. If not one of these conditions NULL
	must be returned.
==============================================================================
*/

typedef const VSFrameRef *(VS_CC *VSFilterGetFrame)(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi);



/*
==============================================================================
Name: *VSFilterFree
Description:
	Plug-in function called by the host to free private filter data,
	passed to the host in createFilter().
Input parameters:
	- instanceData: pointer on the private filter data.
	- core: Pointer on the main host object.
	- vsapi: Pointer on the host function set.
==============================================================================
*/

typedef void (VS_CC *VSFilterFree)(void *instanceData, VSCore *core, const VSAPI *vsapi);



/* other */



/*
==============================================================================
Name: *VSSetFilterError
Description:
	Issues an error from filter getframe methods, and only from them. After
	calling it you must return 0. The error will then bubble up through all
	filters above and be reported.
Input parameters:
	- errorMessage: Error message. Null-terminated string.
Input/output parameters:
	- frameCtx: Pointer on the context where the error should be attached.
==============================================================================
*/



/*
==============================================================================
Name: *VSFrameDoneCallback
Description:
	Function of the client application called by the core when a requested
	frame is ready, after a call to getFrameAsync().
	Only called by one thread at a time to keep it simple.
Input parameters:
	- f: A pointer on the frame data, or 0 on error.
		A new reference is created, so f has to be freed when done with it.
	- n: The frame index, as passed to getFrameAsync().
	- node: A pointer on the node (clip) whose the frame is belonging to.
	- errorMsg: Null-terminated string containing an error message from the
		plug-in if the frame generation failed. 0 if no error.
	- userData: pointer on private data from the client application, as passed
		previously to getFrameAsync().
==============================================================================
*/

typedef void (VS_CC *VSFrameDoneCallback)(void *userData, const VSFrameRef *f, int n, VSNodeRef *, const char *errorMsg);



typedef void (VS_CC *VSMessageHandler)(int msgType, const char *msg, void *userData);

typedef void (VS_CC *VSMessageHandlerFree)(void *userData);

struct VSAPI {

/*
==============================================================================
Name: createCore
Description:
	Creates the Vapoursynth processing core.
	It is possible to create multiple cores.
	The function crashes if it fails.
Input/output parameters:
	- threads: The number of desired working threads. If threads is <= 0, a
		value is automatically chosen.
Returns:
	A pointer on the newly created main host object.
==============================================================================
*/

    VSCore *(VS_CC *createCore)(int threads) VS_NOEXCEPT;



/*
==============================================================================
Name: freeCore
Description:
	Releases all the resources hold by the core.
	All frame requests must be completed.
	All nodes must be freed first, don't forget the ones that can lurk inside
	the VSMap type.
	Frames are still valid with a quirk, the format descriptors have been
	allocated.
	VSMaps without nodes are still valid.
	If these conditions are not met, crash.
Input parameters:
	- core: A pointer on the main host object to delete.
==============================================================================
*/

    void (VS_CC *freeCore)(VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: getCoreInfo
Description:
	Retrieves information on an instantiated core. See VSCoreInfo for details.
Input parameters:
	- core: Pointer on an existing core.
Returns: A pointer to a structure describing the core. Its lifetime is the
	same as the core object.
==============================================================================
*/

    const VSCoreInfo *(VS_CC *getCoreInfo)(VSCore *core) VS_NOEXCEPT; /* deprecated as of api 3.6, use getCoreInfo2 instead */



/*
==============================================================================
Name: cloneFrameRef
Description:
	Duplicates a frame reference. This new reference has to be deleted with
	freeFrame() when no longer in use.
Input parameters:
	- f: A pointer on the frame reference to duplicate.
Returns:
	A new frame reference. Ownership is transmitted to the caller.
==============================================================================
*/

    const VSFrameRef *(VS_CC *cloneFrameRef)(const VSFrameRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: cloneNodeRef
Description:
	Duplicates the reference of a filter instance. This new reference has to be
	deleted with freeNode() when no longer in use.
Input parameters:
	- node: A pointer on the node reference to duplicate.
Returns:
	A new node reference. Ownership is transmitted to the caller.
==============================================================================
*/

    VSNodeRef *(VS_CC *cloneNodeRef)(VSNodeRef *node) VS_NOEXCEPT;



/*
==============================================================================
Name: cloneFuncRef
Description:
	Duplicates the reference of a custom callback function. This new reference
	has to be deleted with freeFunc() when no longer in use.
Input parameters:
	- f: A pointer on the callback reference to duplicate.
Returns:
	A new callback function reference. Ownership is transmitted to the caller.
==============================================================================
*/

    VSFuncRef *(VS_CC *cloneFuncRef)(VSFuncRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: freeFrame
Description:
	Deletes a frame reference, releasing the caller's ownership on the frame.
	Don't try to use the frame once the reference has been deleted.
Input parameters:
	- f: pointer on the frame reference to release. Can be 0 (no effect).
==============================================================================
*/

    void (VS_CC *freeFrame)(const VSFrameRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: freeNode
Description:
	Deletes a filter reference, releasing the caller's ownership on the filter.
	Don't try to use the filter once the reference has been deleted.
Input parameters:
	- node: pointer on the filter reference to release. Can be 0 (no effect).
==============================================================================
*/

    void (VS_CC *freeNode)(VSNodeRef *node) VS_NOEXCEPT;



/*
==============================================================================
Name: freeFunc
Description:
	Deletes a callback function reference, releasing the caller's ownership.
	Don't try to call the function once the reference has been deleted.
Input parameters:
	- f: pointer on the callback function to release. Can be 0 (no effect).
==============================================================================
*/

    void (VS_CC *freeFunc)(VSFuncRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: newVideoFrame
Description:
	Creates a new frame, possibly by copying the characteristics of another
	frame.
Input parameters:
	- format: A pointer on the colorspace format. The structure must have been
		provided by the core by a way or another.
	- width: Width of the frame in pixels, > 0.
	- height: Height of the frame in pixels, > 0.
	- propSrc: A pointer on another frame, whose property map is to be copied
		on the new one. Can be 0 if there is no property to attach.
	- core: Pointer on the main host object.
Returns:
	A pointer on the reference to the created frame. Ownership is transmitted
	to the caller.
==============================================================================
*/

    VSFrameRef *(VS_CC *newVideoFrame)(const VSFormat *format, int width, int height, const VSFrameRef *propSrc, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: copyFrame
Description:
	Duplicates the frame (not just the reference). As the frame buffer and the
	properties are shared in a copy-on-write fashion, the frame content is not
	really duplicated until a write operation occurs. Anyway this is
	transparent for the user.
Input parameters:
	- f: A pointer on a reference to the frame to copy.
	- core: Pointer on the main host object.
Returns:
	A pointer on the reference to the new frame. Ownership is transmitted
	to the caller.
==============================================================================
*/

    VSFrameRef *(VS_CC *copyFrame)(const VSFrameRef *f, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: copyFrameProps
Description:
	Copies the property map of a frame to another frame, owerwriting all the
	previous properties.
Input parameters:
	- src: The frame containing the properties to copy.
	- core: Pointer on the main host object.
Input/output parameters:
	- dst: The frame where the properties have to be copied.
==============================================================================
*/
    void (VS_CC *copyFrameProps)(const VSFrameRef *src, VSFrameRef *dst, VSCore *core) VS_NOEXCEPT;



    void (VS_CC *registerFunction)(const char *name, const char *args, VSPublicFunction argsFunc, void *functionData, VSPlugin *plugin) VS_NOEXCEPT;



/*
==============================================================================
Name: getPluginById
Description:
	Access a plug-in given its unique identifier.
Input parameters:
	- identifier: Null-terminated string.
	- core: a pointer on the main host object.
Returns:
	A pointer on the plug-in, or 0 if not found.
==============================================================================
*/

    VSPlugin *(VS_CC *getPluginById)(const char *identifier, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: getPluginByNs
Description:
	Access a plug-in given its namespace.
Input parameters:
	- ns: namespace, null-terminated string.
	- core: a pointer on the main host object.
Returns:
	A pointer on the first plug-in found belonging to this namespace,
	or 0 if not found.
==============================================================================
*/

    VSPlugin *(VS_CC *getPluginByNs)(const char *ns, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: getPlugins
Description:
	Lists all the referenced plug-ins.
Input parameters:
	- core: a pointer on the main host object.
Returns:
	A map containing the following pairs:
		- key: the plug-in unique identifier.
		- value: namespace, identifier and fullname in this order, separated
			by semicolons.
==============================================================================
*/

    VSMap *(VS_CC *getPlugins)(VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: getFunctions
Description:
	Returns a list of the function declared by the plug-in.
Input parameters:
	- plugin: a pointer on the plug-in to examine.
Returns:
	The map made of pairs of:
		- key: the function name.
		- value: the function name followed by the argument string, separated
			by a semicolon.
==============================================================================
*/

    VSMap *(VS_CC *getFunctions)(VSPlugin *plugin) VS_NOEXCEPT;



/*
==============================================================================
Name: createFilter
Description:
	Host function to be called by the plug-in from the VSProcessArgs fnc.
	It creates a new filter node.
Input parameters:
	- in: Input parameter list. The input arguments are copied into the filter
		on filter creation
	- name: Instance name, should preferably correspond to the simple filter
		name, its relation to the function the user called should be obvious
		so stuff the function name in here please. Null-terminated string.
	- init: The callback function to be called from the host for filter
		initialisation.
	- getFrame: Callback function for frame access.
	- free: a callback to release the instanceData resources when the filter
		is destructed.
	- filterMode: A VSFilterMode value. Indicates the level of parallelism
		supported by the filter.
	- flags: You may set nfNoCache if the filter is unlikely to benefit from
		caching. this is mostly for core filters such as trim, splice, loop
		and so on that simply reorder frames.
	- core: pointer on the main host object, passed previously by the
		VSPublicFunction function.
Output parameters:
	- instanceData: A pointer on the private filter data. The plug-in is
		responsible for resource allocation and deallocation (via the free
		callback). Beware, the nested callback to VSFilterInit() could
		substitute it with another object, so if you need to access the data
		after createFilter() retrun, make sure you use the right pointer.
Input/output parameters:
	- out: Output parameter list. On output, it contains the output properties,
		and more specifically a list of the output clips in the "clip" property.
==============================================================================
*/

    void (VS_CC *createFilter)(const VSMap *in, VSMap *out, const char *name, VSFilterInit init, VSFilterGetFrame getFrame, VSFilterFree free, int filterMode, int flags, void *instanceData, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: setError
Description:
	Adds an error message to a parameter list.
	You should never call setError() twice on the same map, this is just common
	sense. It is also a fatal error to access an errored map with anything
	except getError().
Input parameters:
	- errorMessage: Null-terminated string. 0 for the default error message.
Input/output parameters:
	- map: The map where to put the error message.
==============================================================================
*/

    void (VS_CC *setError)(VSMap *map, const char *errorMessage) VS_NOEXCEPT; /* use to signal errors outside filter getframe functions */



/*
==============================================================================
Name: getError
Description:
	Checks if there is an error contained in a property map and returns the
	error message.
Input parameters:
	- map: A pointer on the map to examine.
Returns:
	A pointer on the error message (null-terminated string), or 0 if there was
	no error.
==============================================================================
*/

    const char *(VS_CC *getError)(const VSMap *map) VS_NOEXCEPT; /* use to query errors, returns 0 if no error */



    void (VS_CC *setFilterError)(const char *errorMessage, VSFrameContext *frameCtx) VS_NOEXCEPT; /* use to signal errors in the filter getframe function */



/*
==============================================================================
Name: invoke
Description:
	Creates a filter.
	Thou shalt only call invoke on filter construction, it is not threadsafe,
	calling it in a getframe method will deadlock.
Input parameters:
	- plugin: A pointer on the plug-in where the filter is located.
	- name: Name of the filter function to create.
	- args: The input argument list, passed to VSPublicFunction in a nested
		calls.
Returns:
	The output parameter map resulting from the call to VSPublicFunction.
	In case of error, it contains a key called "_Error". Use getError() on
	the map to find out.
==============================================================================
*/

    VSMap *(VS_CC *invoke)(VSPlugin *plugin, const char *name, const VSMap *args) VS_NOEXCEPT;



/*
==============================================================================
Name: getFormatPreset
Description:
	Returns a VSFormat structure from a video format identifier.
	Concurrent access allowed with other video format functions.
Input parameters:
	- id: The format identifier, a VSPresetFormat value or a custom registred
		format.
	- core: pointer on the main host object.
Returns:
	A reference on the structure, or 0 if the identifier is not known.
==============================================================================
*/

    const VSFormat *(VS_CC *getFormatPreset)(int id, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: registerFormat
Description:
	Registers a custom video format.
	Concurrent access allowed with other video format functions.
	This function can be used to either query an existing format in a dynamic
	way, for example if you want to extract a single plane and want a frame to
	hold it you can do this:
		registerformat(cmGray, fi->sampleType, fi->bitsPerSample...)
	If the format has already been registered the already registered structure
	is returned.
	Note: getFormatPreset is faster if you know the pfFancyName.
Input parameters:
	- colorFamily: One of the VSColorFamily enum (RGB, YUV, YCoCg or grey)
	- sampleType: if data is integer or floating point. VSSampleType.
	- bitsPerSample: Number of meaningful bits for a single component.
		In range 8-32.
	- subSamplingW: log2 of the horizontal chroma subsampling.
		0 = no subsampling. RGB hasn't got any chroma, hence cannot have
		subsampling.
	- subSamplingH: log2 of the vertical chroma subsampling.
	- core: pointer on the main host object.
Returns:
	A reference on the created VSFormat object. Its id member contains
	the attributed format identifier.
==============================================================================
*/

    const VSFormat *(VS_CC *registerFormat)(int colorFamily, int sampleType, int bitsPerSample, int subSamplingW, int subSamplingH, VSCore *core) VS_NOEXCEPT;



/*
==============================================================================
Name: getFrame
Description:
	Generates a frame directly. The frame is available at the function
	return.
	For external applications using the core as a library or for exceptional
	use if frame requests are necessary during filter initialization.
Input parameters:
	- n: The frame index. >= 0, otherwise it crashes.
	- node: A pointer on the node on which the frame is requested.
	- bufSize: maximum length for the error message, in bytes (including the
		trailing '\0'). 0 allowed for no error message output.
	- errorMsg: Pointer to a buffer of bufSize bytes to store a possible error
		message.
Returns:
	A reference on the generated frame, or 0 in case of failure.
	The ownership of the frame is transmitted to the caller, therefore
	it has to delete its reference when it has finished using it.
==============================================================================
*/

    const VSFrameRef *(VS_CC *getFrame)(int n, VSNodeRef *node, char *errorMsg, int bufSize) VS_NOEXCEPT; /* do never use inside a filter's getframe function, for external applications using the core as a library or for requesting frames in a filter constructor */



/*
==============================================================================
Name: getFrameAsync
Description:
	Requests the generation of a frame. When it is ready, a user-provided
	callback function is called.
	For external applications using the core as a library.
	Thread-safe, can be called concurrently.
Input parameters:
	- n: Frame number. >= 0, otherwise it crashes.
	- node: A pointer on the node on which the frame is requested.
	- callback: a pointer on the callback function.
	- userData: pointer passed to the callback.
==============================================================================
*/

    void (VS_CC *getFrameAsync)(int n, VSNodeRef *node, VSFrameDoneCallback callback, void *userData) VS_NOEXCEPT; /* do never use inside a filter's getframe function, for external applications using the core as a library or for requesting frames in a filter constructor */


/*
==============================================================================
Name: getFrameFilter
Description:
	Host function called by the plug-in from the processing function of a
	filter. Retrieves an input frame that has been previously requested.
	Generally called in the arAllFramesReady state.
Input parameters:
	- n: The frame index. >= 0.
	- node: Pointer on the filter retrieving the frame.
Input/output parameters:
	- frameCtx: The context passed to VSFilterGetFrame plug-in function.
Returns:
	A pointer on the frame reference, or 0 if it is not available (yet).
	The ownership of the frame is transmitted to the caller so the reference
	should be deleted when it has finished using it, or shoud be transmitted
	to another function.
==============================================================================
*/

    const VSFrameRef *(VS_CC *getFrameFilter)(int n, VSNodeRef *node, VSFrameContext *frameCtx) VS_NOEXCEPT; /* only use inside a filter's getframe function */



/*
==============================================================================
Name: requestFrameFilter
Description:
	Host function called by the plug-in from the processing function of a
	filter. Requests an input frame when the activation reason is set to
	arInitial. The function returns almost immediately. The frame data is
	available later in a subsequent call to the VSFilterGetFrame plug-in
	function and can be retrieved by a call to getFrameFilter().
	Only use inside a filter's getframe function.
	The function can be called in arFrameReady or arAllFramesReady states too,
	extending the request cycle. The only exception being arError of course
	where it's a mortal sin.
	Try to keep requests in order as it usually help throughput of upstream
	filters. Example: request frames 1 2 3, not 3 1 2.
Input parameters:
	- n: The frame index. >= 0, try to keep it within the clip length if
		defined, this applies everywhere.
	- node: Pointer on the filter requesting the frame
Input/output parameters:
	- frameCtx: The context passed to VSFilterGetFrame plug-in function.
==============================================================================
*/

    void (VS_CC *requestFrameFilter)(int n, VSNodeRef *node, VSFrameContext *frameCtx) VS_NOEXCEPT; /* only use inside a filter's getframe function */



    void (VS_CC *queryCompletedFrame)(VSNodeRef **node, int *n, VSFrameContext *frameCtx) VS_NOEXCEPT; /* only use inside a filter's getframe function */



    void (VS_CC *releaseFrameEarly)(VSNodeRef *node, int n, VSFrameContext *frameCtx) VS_NOEXCEPT; /* only use inside a filter's getframe function */



/*
==============================================================================
Name: getStride
Description:
	Gets the offset in bytes between two lines of a plane of a frame.
Input parameters:
	- f: The accessed frame.
	- plane: Plane index, >= 0. Upper bound depends on the colorspace format
		(1 or 3 plane)
Returns: The stride, as a number of bytes (not pixel values).
==============================================================================
*/

    int (VS_CC *getStride)(const VSFrameRef *f, int plane) VS_NOEXCEPT;



/*
==============================================================================
Name: getReadPtr
Description:
	Gives access in read-only to the plane data of a given frame.
Input parameters:
	- f: The accessed frame.
	- plane: Plane index, >= 0.
Returns: A pointer on the frame data, on the top-left pixel of the frame.
==============================================================================
*/

    const uint8_t *(VS_CC *getReadPtr)(const VSFrameRef *f, int plane) VS_NOEXCEPT;



/*
==============================================================================
Name: getWritePtr
Description:
	Gives access in read/write to the plane data of a given frame.
Input parameters:
	- f: The accessed frame.
	- plane: Plane index, >= 0.
Returns: A pointer on the frame data, on the top-left pixel of the frame.
==============================================================================
*/

    uint8_t *(VS_CC *getWritePtr)(VSFrameRef *f, int plane) VS_NOEXCEPT;



    VSFuncRef *(VS_CC *createFunc)(VSPublicFunction func, void *userData, VSFreeFuncData free, VSCore *core, const VSAPI *vsapi) VS_NOEXCEPT;


    void (VS_CC *callFunc)(VSFuncRef *func, const VSMap *in, VSMap *out, VSCore *core, const VSAPI *vsapi) VS_NOEXCEPT; /* core and vsapi arguments are completely ignored, they only remain to preserve ABI */



    /* property access functions */



/*
==============================================================================
Name: VSCreateMap
Description:
	Creates a property map.
Returns: A pointer on the map. It must be deallocated later with freeMap().
==============================================================================
*/

    VSMap *(VS_CC *createMap)(void) VS_NOEXCEPT;



/*
==============================================================================
Name: freeMap
Description:
	Free an allocated map and all its contained objects.
	The pointer is invalid after the function returned.
Input parameters:
	- map: Pointer on the map to free.
==============================================================================
*/

    void (VS_CC *freeMap)(VSMap *map) VS_NOEXCEPT;



/*
==============================================================================
Name: clearMap
Description:
	Deletes all the (key, value) pairs contained in the map, leaving the
	map empty.
Input/output parameters:
	- map: A pointer on the map to clear.
==============================================================================
*/

    void (VS_CC *clearMap)(VSMap *map) VS_NOEXCEPT;



/*
==============================================================================
Name: getVideoInfo
Description:
	Returns the video format associated to the output of a filter instance.
Input parameters:
	- node: A pointer to the filter instance.
Returns:
	A pointer to the video information. Its lifetime is related to the node.
==============================================================================
*/

    const VSVideoInfo *(VS_CC *getVideoInfo)(VSNodeRef *node) VS_NOEXCEPT;



/*
==============================================================================
Name: setVideoInfo
Description:
	Host function called by the plug-in during the filter initialisation in
	its implementation of the VSFilterInit function (and only there).
	It sets the video format information.
	It is a fatal error to not set it in a filter constructor (unless you
	error out of course).
Input parameters:
	- vi: Pointer on the video information, properly filled. The structure is
		copied by the host.
	- numOutputs: number of output video streams, positive (can be 0 ???).
		The new clips are created later on the return of the createFilter() call
		and inserted in the output properties in the "clip" array.
Input/output parameters:
	- node: A pointer on the filter instance where the video information
		has to be set.
==============================================================================
*/

    void (VS_CC *setVideoInfo)(const VSVideoInfo *vi, int numOutputs, VSNode *node) VS_NOEXCEPT;



/*
==============================================================================
Name: getFrameFormat
Description:
	Retrieves the colorspace format of a frame.
Input parameters:
	- f: Frame to examine.
Returns:
	A pointer on the format. Lifetime tied to the core.
==============================================================================
*/

    const VSFormat *(VS_CC *getFrameFormat)(const VSFrameRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: getFrameWidth
Description:
	Gets the width of plane of a given frame. The width depends on the plane
	because of the possible chroma subsampling.
Input parameters:
	- f: The desired frame.
	- plane: The desired plane index.
Returns: The frame width, in pixels.
==============================================================================
*/

    int (VS_CC *getFrameWidth)(const VSFrameRef *f, int plane) VS_NOEXCEPT;



/*
==============================================================================
Name: getFrameHeight
Description:
	Gets the height of plane of a given frame. The height depends on the plane
	because of the possible chroma subsampling.
Input parameters:
	- f: The desired frame.
	- plane: The desired plane index.
Returns:The frame height, in pixels.
==============================================================================
*/

    int (VS_CC *getFrameHeight)(const VSFrameRef *f, int plane) VS_NOEXCEPT;



/*
==============================================================================
Name: getFramePropsRO
Description:
	Access the frame properties in read-only mode.
Input parameters:
	- f: The desired frame.
Returns:
	A reference on the frame property map. Its lifetime depends on the frame.
==============================================================================
*/

    const VSMap *(VS_CC *getFramePropsRO)(const VSFrameRef *f) VS_NOEXCEPT;



/*
==============================================================================
Name: getFramePropsRW
Description:
	Access the frame properties with writing permissions.
Input parameters:
	- f: The desired frame.
Returns:
	A reference on the frame property map that can be modified. Its lifetime
	depends on the frame.
==============================================================================
*/

    VSMap *(VS_CC *getFramePropsRW)(VSFrameRef *f) VS_NOEXCEPT;

/*
==============================================================================
Name: propNumKeys
Description:
	Counts the keys contained in a property map. Only valid until the map
	is modified.
Input parameters:
	- map: A pointer on the map.
Returns: The number of keys.
==============================================================================
*/



/*
==============================================================================
Name: propGetKey
Description:
	Gets the name of a given key in a map.
Input parameters:
	- map: A pointer on the map.
	- index: Index of the key. Between 0 and propNumKeys() - 1.
Returns:
	A pointer on the name of the key, null-terminated string. The lifetime
	of the result depends on the existence of the key. Only valid until the
	map is modified, of course.
==============================================================================
*/



/*
==============================================================================
Name: propNumElements
Description:
	Gets the number of elements associated to a single key in a property map.
Input parameters:
	- map: A pointer on the map.
	- key: The name of the key, null-terminated string.
Returns:
	The number of elements (can be 0), or -1 if the key doesn't exist.
==============================================================================
*/



/*
==============================================================================
Name: propGetType
Description:
	Gets the type of the element(s) associates to the given key in a property
	map.
Input parameters:
	- map: A pointer on the map.
	- key: The name of the key, null-terminated string.
Returns:
	One of the following values:
		- 'u': unknown key or unknown type
		- 'i': 64-bit integer
		- 'f': double
		- 's': string/data
		- 'c': node
		- 'v': frame
==============================================================================
*/




    int (VS_CC *propNumKeys)(const VSMap *map) VS_NOEXCEPT;
    const char *(VS_CC *propGetKey)(const VSMap *map, int index) VS_NOEXCEPT;
    int (VS_CC *propNumElements)(const VSMap *map, const char *key) VS_NOEXCEPT;
    char (VS_CC *propGetType)(const VSMap *map, const char *key) VS_NOEXCEPT;



/*
==============================================================================
Name:
	propGetInt
	propGetFloat
	propGetData
	propGetDataSize
	propGetNode
	propGetFrame
	propGetFunc
Description:
	Retrieves a property from an argument map.
	propGetDataSize returns the size in bytes of a property of "data" type in a
	map.
Input parameters:
	- map: The map where the key has to be read.
	- key: Name of the key to read. Null-terminated string.
	- index: Index of the element when multiple values are associated to a
		single key.
		Use propNumElements() to know the total number of elements.
		Set index to 0 when only one element is associated to the given key.
Output parameters:
	- error: pointer to an int that gets 0 in case of success, or a
		value > 0 in case of error. It is a bit combination of the
		GetPropErrors enum. It is a fatal error to get an error and not supply
		an &int for error reporting.
Returns:
	The value or a pointer on the value on success, or 0 in case of error.
	Strings/data are just communicated as a reference. The object lasts until
	the map destruction (or explicit removal from the map).
	VSNodeRef and VSFrameRef objects are duplicated, so they have to be
	deallocated after use by the caller with freeNode() or freeFrame().
==============================================================================
*/

    int64_t(VS_CC *propGetInt)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    double(VS_CC *propGetFloat)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    const char *(VS_CC *propGetData)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    int (VS_CC *propGetDataSize)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    VSNodeRef *(VS_CC *propGetNode)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    const VSFrameRef *(VS_CC *propGetFrame)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;
    VSFuncRef *(VS_CC *propGetFunc)(const VSMap *map, const char *key, int index, int *error) VS_NOEXCEPT;



/*
==============================================================================
Name: propDeleteKey
Description:
	In a property map, removes all values associated with a given key.
Input parameters:
	- key: Name of the key to delete. Null-terminated string.
Input/output parameters:
	- map: Property map where the deletion should take place.
Returns: 1 if the key exists, 0 if it doesn't.
==============================================================================
*/

    int (VS_CC *propDeleteKey)(VSMap *map, const char *key) VS_NOEXCEPT;



/*
==============================================================================
Name:
	propSetInt
	propSetFloat
	propSetData
	propSetNode
	propSetFrame
	propSetFunc
Description:
	Adds a (key, value) pair to a property map.
	The map can have multiple values for the same key (thus defining an array),
	but all the values must be of the same type.
Input parameters:
	- key: name of the property. Null-terminated string. Do try to stick to
		the usual 'a'-'z', 'A'-'Z', '0'-'9' stuff. '_' is reserved for system-
		defined properties.
	- i, d, data, node, f: Data to store. Data referenced by pointers are
		copied so you can deallocate the initial data if not needed anymore.
	- size: for "data" type, indicates the exact number of bytes to copy.
		If set to 0, data is considered as a null-terminated string.
		Note that a magic '\0' is always appended to the data, so you will
		never overread data if you accidentally handle it as a string.
	- append: One of the PropAppendMode enum:
		- paReplace: replaces all existing value for key by the new value.
		- paAppend: just adds the value to the existing ones ("push back").
		- paTouch: creates an empty vector set but no values in it.
Input/output parameters:
	- map: The map where the (key, value) pair must be inserted.
Returns:
	0 if success
	1 if failed. Namely when trying to append a property with a different type.
==============================================================================
*/

    int (VS_CC *propSetInt)(VSMap *map, const char *key, int64_t i, int append) VS_NOEXCEPT;
    int (VS_CC *propSetFloat)(VSMap *map, const char *key, double d, int append) VS_NOEXCEPT;
    int (VS_CC *propSetData)(VSMap *map, const char *key, const char *data, int size, int append) VS_NOEXCEPT;
    int (VS_CC *propSetNode)(VSMap *map, const char *key, VSNodeRef *node, int append) VS_NOEXCEPT;
    int (VS_CC *propSetFrame)(VSMap *map, const char *key, const VSFrameRef *f, int append) VS_NOEXCEPT;
    int (VS_CC *propSetFunc)(VSMap *map, const char *key, VSFuncRef *func, int append) VS_NOEXCEPT;

    int64_t (VS_CC *setMaxCacheSize)(int64_t bytes, VSCore *core) VS_NOEXCEPT;
    int (VS_CC *getOutputIndex)(VSFrameContext *frameCtx) VS_NOEXCEPT;



/*
==============================================================================
Name: newVideoFrame2
Description:
	Creates a new frame, possibly by copying the characteristics of another
	frame. This function can reuse the planes from other frames.
Input parameters:
	- format: A pointer on the colorspace format. The structure must have been
		provided by the core by a way or another.
	- width: Width of the frame in pixels, > 0.
	- height: Height of the frame in pixels, > 0.
	- planeSrc: An array of frame pointers, one frame per plane of the new
		frame. The new frame is constructed with planes coming from these
		referenced frames. These planes are not writable!
		Set the pointer to 0 if you want a fresh uninitialized plane.
	- planes: An array of plane indexes. Indicates which planes to use from
		the corresponding frame declared in planeSrc. If the specified plane
		doesn't exist or if it has the wrong dimension, the program exits with
		a fatal error.
	- propSrc: A pointer on another frame, whose property map is to be copied
		on the new one. Can be 0 if there is no property to attach.
	- core: Pointer on the main host object.
Returns:
	A pointer on the reference to the created frame. Ownership is transmitted
	to the caller.
==============================================================================
*/

    VSFrameRef *(VS_CC *newVideoFrame2)(const VSFormat *format, int width, int height, const VSFrameRef **planeSrc, const int *planes, const VSFrameRef *propSrc, VSCore *core) VS_NOEXCEPT;
    void (VS_CC *setMessageHandler)(VSMessageHandler handler, void *userData) VS_NOEXCEPT; /* deprecated as of api 3.6, use addMessageHandler and removeMessageHandler instead */
    int (VS_CC *setThreadCount)(int threads, VSCore *core) VS_NOEXCEPT;

    const char *(VS_CC *getPluginPath)(const VSPlugin *plugin) VS_NOEXCEPT;

    /* api 3.1 */
    const int64_t *(VS_CC *propGetIntArray)(const VSMap *map, const char *key, int *error) VS_NOEXCEPT;
    const double *(VS_CC *propGetFloatArray)(const VSMap *map, const char *key, int *error) VS_NOEXCEPT;

    int (VS_CC *propSetIntArray)(VSMap *map, const char *key, const int64_t *i, int size) VS_NOEXCEPT;
    int (VS_CC *propSetFloatArray)(VSMap *map, const char *key, const double *d, int size) VS_NOEXCEPT;

    /* api 3.4 */
    void (VS_CC *logMessage)(int msgType, const char *msg) VS_NOEXCEPT;

    /* api 3.6 */
    int (VS_CC *addMessageHandler)(VSMessageHandler handler, VSMessageHandlerFree free, void *userData) VS_NOEXCEPT;
    int (VS_CC *removeMessageHandler)(int id) VS_NOEXCEPT;
    void (VS_CC *getCoreInfo2)(VSCore *core, VSCoreInfo *info) VS_NOEXCEPT;
};

VS_API(const VSAPI *) getVapourSynthAPI(int version) VS_NOEXCEPT;

#endif /* VAPOURSYNTH_H */
