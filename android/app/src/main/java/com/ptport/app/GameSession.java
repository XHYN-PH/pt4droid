package com.ptport.app;

import android.content.Context;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;

/** A process-safe guard for saves while the native game is alive. */
final class GameSession implements AutoCloseable {
    private final RandomAccessFile file;
    private final FileLock lock;

    private GameSession(RandomAccessFile file, FileLock lock) {
        this.file = file;
        this.lock = lock;
    }

    static GameSession start(Context context) throws IOException {
        RandomAccessFile file = new RandomAccessFile(new File(context.getFilesDir(), "game-session.lock"), "rw");
        try {
            FileLock lock = file.getChannel().tryLock();
            if (lock == null) throw new IOException("The game is already running or closing.");
            return new GameSession(file, lock);
        } catch (IOException | OverlappingFileLockException error) {
            file.close();
            throw new IOException("The game is already running or its save guard could not be opened.", error);
        }
    }

    static boolean isActive(Context context) {
        try (GameSession probe = start(context)) {
            return false;
        } catch (IOException error) {
            // Fail closed: never allow save deletion if the guard cannot be checked.
            return true;
        }
    }

    @Override public void close() throws IOException {
        try { if (lock.isValid()) lock.release(); }
        finally { file.close(); }
    }
}
