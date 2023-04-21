import { useCallback, useEffect, useLayoutEffect, useRef, useState } from 'react';
import { invoke } from '@tauri-apps/api/tauri';
import { listen } from '@tauri-apps/api/event';
import { Button, Input, Textarea } from '@material-tailwind/react';
import { event } from '@tauri-apps/api';

const maxConsoleLines = 200;

const now = () => new Date().toISOString().replace('T', ' ').slice(0, -1);

const appendBuffer = (buffer: string[], ...lines: string[]) => {
    if (buffer.length >= maxConsoleLines) {
        const newBuffer = buffer.slice(lines.length);
        newBuffer.push(...lines);
        return newBuffer;
    }

    return [...buffer, ...lines];
};

function Console() {
    const [consoleBuffer, setConsoleBuffer] = useState<string[]>([]);
    const [command, setCommand] = useState('');

    const consoleRef = useRef<HTMLInputElement>(null);

    const commandTrimmed = command.trim();

    useEffect(() => {
        const unlistenLog = listen('log', (event: event.Event<string>) => {
            console.log({ event });
            setConsoleBuffer((buffer) => appendBuffer(buffer, `[${now()}] ${event.payload}`));
        });

        console.log('initialized');

        return () => {
            Promise.all([unlistenLog]).then((unlistenAll) => {
                unlistenAll.forEach((unlisten) => unlisten());
                console.log('uninitialized');
            });
        };
    }, []);

    useLayoutEffect(() => {
        if (consoleRef.current?.firstElementChild) {
            consoleRef.current.firstElementChild.scrollTop = consoleRef.current.firstElementChild.scrollHeight;
        }
    }, [consoleBuffer]);

    const onClickExecute = useCallback(() => {
        const text = command;

        const updateBuffer = (line: unknown) => {
            setConsoleBuffer((buffer) => appendBuffer(buffer, `[${now()}] > ${text}`, `[${now()}] ${line}`));
        };

        invoke('console_execute', { text }).then(updateBuffer).catch(updateBuffer);

        setCommand('');
    }, [setCommand, command]);

    const onChangeCommand = useCallback(({ target }: any) => {
        setCommand(target.value as string);
    }, []);

    const onKeyDownCommand = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter') {
                onClickExecute();
            }
        },
        [onClickExecute],
    );

    return (
        <>
            <div className="flex pb-2">
                <Textarea
                    ref={consoleRef}
                    value={consoleBuffer.join('\n')}
                    className="!border-t-blue-gray-200 focus:!border-t-blue-500 h-[385px] font-['Consolas']"
                    labelProps={{
                        className: 'before:content-none after:content-none',
                    }}
                    readOnly
                />
            </div>
            <div className="relative w-full">
                <Input
                    value={command}
                    onChange={onChangeCommand}
                    className="!border-t-blue-gray-200 focus:!border-t-blue-500 font-['Consolas']"
                    labelProps={{
                        className: 'before:content-none after:content-none',
                    }}
                    containerProps={{
                        className: 'min-w-0',
                    }}
                    onKeyDown={commandTrimmed ? onKeyDownCommand : undefined}
                />
                <Button
                    size="sm"
                    color={commandTrimmed ? 'blue' : 'blue-gray'}
                    disabled={!commandTrimmed}
                    className="!absolute right-1 top-1 rounded"
                    onClick={onClickExecute}
                >
                    Execute
                </Button>
            </div>
        </>
    );
}

export default Console;
