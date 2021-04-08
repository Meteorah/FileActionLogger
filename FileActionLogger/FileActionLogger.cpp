#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <ctime>

void WatchDirectory(LPTSTR);

void _tmain(int argc, TCHAR* argv[])
{
    if (argc != 2)
    {
        _tprintf(TEXT("Usage: %s <dir>\n"), argv[0]);
        return;
    }

    WatchDirectory(argv[1]);
}

DWORD processDirectoryChanges(const char* buffer)
{
    DWORD offset = 0;
    char fileName[MAX_PATH] = "";
    FILE_NOTIFY_INFORMATION* fni = NULL;

    do
    {
        fni = (FILE_NOTIFY_INFORMATION*)(&buffer[offset]);
        // since we do not use UNICODE, 
        // we must convert fni->FileName from UNICODE to multibyte
        int ret = WideCharToMultiByte(CP_ACP, 0, fni->FileName,
            fni->FileNameLength / sizeof(WCHAR),
            fileName, sizeof(fileName), NULL, NULL);

        time_t rawtime;
        tm timeinfo;
        char currentTime[80];
        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);
        strftime(currentTime, sizeof(currentTime), "%d-%m-%Y %H:%M:%S", &timeinfo);
        std::string strCurrentTime(currentTime);

        // Logging file creates infinite loop
        /*char loggedLine[200];
        std::ofstream outfile;
        outfile.open("FileActionLog.txt", std::ios::app);
        if (!outfile.is_open()) ExitProcess(GetLastError());*/

        switch (fni->Action) {
        case FILE_ACTION_ADDED: {
            std::cout << strCurrentTime << ": " << "Added file " << fileName << std::endl;
            //outfile << strCurrentTime << ": " << "Added file " << fileName << std::endl;
            break;
        } case FILE_ACTION_REMOVED: {
            std::cout << strCurrentTime << ": " << "Removed file " << fileName << std::endl;
            //outfile << strCurrentTime << ": " << "Removed file " << fileName << std::endl;
            break;
        } case FILE_ACTION_MODIFIED: {
            std::cout << strCurrentTime << ": " << "Modified file " << fileName << std::endl;
            //outfile << strCurrentTime << ": " << "Modified file " << fileName << std::endl;
            break;
        } case FILE_ACTION_RENAMED_OLD_NAME: {
            std::cout << strCurrentTime << ": " << "The file was renamed and the old file name is " << fileName << std::endl;
            //outfile << strCurrentTime << ": " << "The file was renamed and the old file name is " << fileName << std::endl;
            break;
        } case FILE_ACTION_RENAMED_NEW_NAME: {
            std::cout << strCurrentTime << ": " << "The file was renamed and the new file name is " << fileName << std::endl;
            //outfile << strCurrentTime << ": " << "The file was renamed and the new file name is " << fileName << std::endl;
            break;
        } default:
            break;
        }
        //outfile.close();
        memset(fileName, '\0', sizeof(fileName));
        offset += fni->NextEntryOffset;

    } while (fni->NextEntryOffset != 0);

    return 0;
}


void WatchDirectory(LPTSTR lpDir)
{
    DWORD dwWaitStatus;

    // Watch the directory for file creation and deletion. 
    HANDLE curDirectory = CreateFileW(
        lpDir,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (curDirectory == INVALID_HANDLE_VALUE)
    {
        printf("\n ERROR: CreateFile Failed.\n");
        ExitProcess(GetLastError());
    }


    OVERLAPPED ovl = { 0 };
    ovl.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    if (NULL == ovl.hEvent) {
        printf("\n ERROR: ovlhEvent failed.\n");
        ExitProcess(GetLastError());
    }

    char buffer[1024];

    BOOL success = ReadDirectoryChangesW(
        curDirectory,
        buffer,
        sizeof(buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE
        | FILE_NOTIFY_CHANGE_CREATION
        | FILE_NOTIFY_CHANGE_FILE_NAME,
        NULL,
        &ovl,
        NULL);

    // Change notification is set. Now wait on both notification 
    // handles and refresh accordingly.
    if (success == 0) {
        printf("\n ERROR: ReadDirectoryChangesW failed.\n");
        ExitProcess(GetLastError());
    }

    while (TRUE)
    {
        // Wait for notification.

        printf("\nWaiting for notification...\n");
        DWORD dw;
        dwWaitStatus = WaitForSingleObject(
            ovl.hEvent,
            INFINITE
        );

        switch (dwWaitStatus)
        {
        case WAIT_TIMEOUT:

            // DO nothing
            break;

        case WAIT_OBJECT_0:

            GetOverlappedResult(curDirectory, &ovl, &dw, FALSE);

            processDirectoryChanges(buffer);

            ResetEvent(ovl.hEvent);

            ReadDirectoryChangesW(curDirectory,
                buffer, sizeof(buffer), FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME,
                NULL, &ovl, NULL);

            break;
        }
    }
}