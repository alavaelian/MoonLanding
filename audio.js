mergeInto(LibraryManager.library, {
    web_audio_init: function() {
        window._audioCtx = null;
        window._sounds = {};
        window._rawData = {};
        window._playing = {};
        
        function startAudio() {
            if (window._audioCtx) return;
            window._audioCtx = new (window.AudioContext || window.webkitAudioContext)();
            window._audioCtx.resume();
            // Decodificar todos los raw pendientes
            Object.keys(window._rawData).forEach(function(id) {
                window._audioCtx.decodeAudioData(window._rawData[id]).then(function(decoded) {
                    window._sounds[id] = decoded;
                    console.log('Decoded: ' + id);
                    if (id === 'splash') {
                        var src = window._audioCtx.createBufferSource();
                        var g = window._audioCtx.createGain();
                        src.buffer = decoded;
                        src.loop = true;
                        g.gain.value = 1.0;
                        src.connect(g);
                        g.connect(window._audioCtx.destination);
                        src.start(0);
                        var obj = {};
                        obj.source = src;
                        obj.gain = g;
                        window._playing['splash'] = obj;
                    }
                });
            });
            document.removeEventListener('keydown', startAudio);
            document.removeEventListener('click', startAudio);
        }
        document.addEventListener('keydown', startAudio);
        document.addEventListener('click', startAudio);
    },
    
    web_audio_load: function(idPtr, pathPtr) {
        var id = UTF8ToString(idPtr);
        var path = UTF8ToString(pathPtr);
        try {
            var data = FS.readFile(path);
            window._rawData[id] = data.buffer.slice(0);
            console.log('Raw audio read: ' + id);
        } catch(e) {
            console.log('Audio file not found: ' + path + ' ' + e);
        }
    },
    
    web_audio_play: function(idPtr, loop, volume) {
        var id = UTF8ToString(idPtr);
        if (!window._audioCtx) return;
        var ctx = window._audioCtx;
        if (ctx.state === 'suspended') ctx.resume();
        
        function doPlay(decoded) {
            if (window._playing[id]) {
                try { window._playing[id].source.stop(); } catch(e) {}
            }
            var src = ctx.createBufferSource();
            var g = ctx.createGain();
            src.buffer = decoded;
            src.loop = loop ? true : false;
            g.gain.value = volume;
            src.connect(g);
            g.connect(ctx.destination);
            src.start(0);
            var obj = {};
            obj.source = src;
            obj.gain = g;
            window._playing[id] = obj;
        }
        
        if (window._sounds[id]) {
            doPlay(window._sounds[id]);
        } else if (window._rawData[id]) {
            ctx.decodeAudioData(window._rawData[id]).then(function(decoded) {
                window._sounds[id] = decoded;
                doPlay(decoded);
            });
        }
    },
    
    web_audio_stop: function(idPtr) {
        var id = UTF8ToString(idPtr);
        if (window._playing[id]) {
            try { window._playing[id].source.stop(); } catch(e) {}
            delete window._playing[id];
        }
    }
});
