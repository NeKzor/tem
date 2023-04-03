/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

import { createElement, useCallback, useEffect, useMemo, useState } from 'react';
import { invoke } from '@tauri-apps/api/tauri';
import {
    Button,
    Card,
    CardHeader,
    CardBody,
    CardFooter,
    Input,
    Typography,
    Tooltip,
    IconButton,
    Tabs,
    TabsHeader,
    TabsBody,
    Tab,
    TabPanel,
    Textarea,
    Select,
    Option,
    Menu,
    MenuHandler,
    MenuList,
    MenuItem,
    Dialog,
    DialogHeader,
    DialogBody,
    DialogFooter,
} from '@material-tailwind/react';
import {
    Bars3Icon,
    ChevronRightIcon,
    CommandLineIcon,
    PencilIcon,
    WrenchIcon,
    RocketLaunchIcon,
    TrashIcon,
    XMarkIcon,
} from '@heroicons/react/24/solid';

interface LauncherMod {
    name: string;
    version: string;
}

interface LauncherConfig {
    name: string;
    resolution: {
        w: number;
        h: number;
    };
    isFullscreen: boolean;
    showSplashScreen: boolean;
    mods: LauncherMod[];
}

const testConfigs: LauncherConfig[] = [
    {
        name: "NeKz's Debug Launch Config",
        resolution: {
            w: 1280,
            h: 720,
        },
        isFullscreen: false,
        showSplashScreen: false,
        mods: [
            {
                name: 'TEM',
                version: 'pre-0.1.0-20230401',
            },
            {
                name: 'XDead',
                version: 'pre-0.1.0',
            },
        ],
    },
    {
        name: 'Fullscreen Mode',
        resolution: {
            w: 1920,
            h: 1080,
        },
        isFullscreen: true,
        showSplashScreen: false,
        mods: [
            {
                name: 'TEM',
                version: 'pre-0.1.0-20230401',
            },
            {
                name: 'XDead',
                version: 'pre-0.1.0',
            },
        ],
    },
];

function App() {
    const [consoleBuffer, setConsoleBuffer] = useState<string[]>([]);
    const [command, setcommand] = useState('');
    const [checkForUpdates, setCheckForUpdates] = useState('disabled');

    const [configs, setConfigs] = useState<LauncherConfig[]>([]);
    const [editDialog, setEditDialog] = useState(false);
    const [configToDelete, setConfigToDelete] = useState('');
    const [deleteDialog, setDeleteDialog] = useState(false);

    const onClickEdit = useCallback(() => setEditDialog(true), [setEditDialog]);
    const onClickCloseEditDialog = useCallback(() => {
        setEditDialog(false);
    }, [setEditDialog]);

    const onChangeDelete = useCallback(() => setDeleteDialog(true), [setDeleteDialog]);
    const onClickCloseDeleteDialog = useCallback(() => {
        setConfigToDelete('');
        setDeleteDialog(false);
    }, [setConfigToDelete, setDeleteDialog]);

    const onChangeConfigToDelete = useCallback(({ target }: any) => {
        setConfigToDelete(target.value as string);
    }, []);

    const onClickDeleteConfig = useCallback(() => {
        invoke('config.delete', { name: configToDelete })
            .then(() => {})
            .catch((error) => setConsoleBuffer([...consoleBuffer, error as string]))
            .finally(() => {
                setConfigToDelete('');
                setDeleteDialog(false);
            });
    }, [setConfigToDelete, setDeleteDialog]);

    const canConfirmDeletion = useMemo(() => {
        return configToDelete === "NeKz's Debug Launch Config";
    }, [configToDelete]);

    const onKeyDownConfigToDelete = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter' && canConfirmDeletion) {
                onClickDeleteConfig();
            }
        },
        [onClickDeleteConfig, canConfirmDeletion],
    );

    useEffect(() => {
        invoke('launcher.configs')
            .then((configs) => setConfigs(configs as LauncherConfig[]))
            .catch((error) => setConsoleBuffer([...consoleBuffer, error as string]))
            .finally(() => setConfigs(testConfigs));
    }, [setConfigs, setConsoleBuffer]);

    useEffect(() => {
        invoke('console.buffer')
            .then((newConsoleBuffer) => setConsoleBuffer([...consoleBuffer, ...(newConsoleBuffer as string[])]))
            .catch((error) => setConsoleBuffer([...consoleBuffer, error as string]));
    }, [setConsoleBuffer]);

    const onClickExecute = useCallback(() => {
        invoke('console.execute', { command })
            .then((line) => setConsoleBuffer([...consoleBuffer, line as string]))
            .catch((error) => setConsoleBuffer([...consoleBuffer, error as string]));
        setcommand('');
    }, [setcommand, command]);

    const onChangeCommand = useCallback(({ target }: any) => {
        setcommand(target.value as string);
    }, []);

    const onKeyDownCommand = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter') {
                onClickExecute();
            }
        },
        [onClickExecute],
    );

    const onChangeCheckForUpdates = useCallback(
        (value: any) => {
            setCheckForUpdates(value);
        },
        [setCheckForUpdates],
    );

    return (
        <div className="container">
            <main>
                <Tabs value="tem">
                    <TabsHeader>
                        <Tab key="tem" value="tem">
                            <div className="flex items-center gap-2">
                                {createElement(RocketLaunchIcon, { className: 'w-5 h-5' })}
                                Launcher
                            </div>
                        </Tab>
                        <Tab key="console" value="console">
                            <div className="flex items-center gap-2">
                                {createElement(CommandLineIcon, { className: 'w-5 h-5' })}
                                Console
                            </div>
                        </Tab>
                        <Tab key="settings" value="settings">
                            <div className="flex items-center gap-2">
                                {createElement(WrenchIcon, { className: 'w-5 h-5' })}
                                Settings
                            </div>
                        </Tab>
                    </TabsHeader>
                    <TabsBody>
                        <TabPanel key="tem" value="tem">
                            <div className="grid grid-cols-3 gap-4">
                                {configs.map((config) => {
                                    return (
                                        <div key={config.name}>
                                            <Card className="w-full max-w-[26rem] shadow-lg">
                                                <CardBody>
                                                    <div className="mb-3 flex items-center justify-between">
                                                        <Typography
                                                            variant="h5"
                                                            color="blue-gray"
                                                            className="font-medium"
                                                        >
                                                            {config.name}
                                                        </Typography>
                                                        <Menu placement="bottom-end">
                                                            <MenuHandler>
                                                                <IconButton
                                                                    color="blue-gray"
                                                                    variant="text"
                                                                    className="!absolute top-4 right-4 rounded-full"
                                                                >
                                                                    <Bars3Icon className="h-6 w-6" />
                                                                </IconButton>
                                                            </MenuHandler>
                                                            <MenuList>
                                                                <MenuItem
                                                                    className="flex items-center gap-2"
                                                                    onClick={onClickEdit}
                                                                >
                                                                    <PencilIcon strokeWidth={2} className="h-4 w-4" />
                                                                    <Typography variant="small" className="font-normal">
                                                                        Edit
                                                                    </Typography>
                                                                </MenuItem>
                                                                <MenuItem
                                                                    className="flex items-center gap-2 hover:bg-red-500"
                                                                    onClick={onChangeDelete}
                                                                >
                                                                    <TrashIcon strokeWidth={2} className="h-4 w-4" />
                                                                    <Typography variant="small" className="font-normal">
                                                                        Delete
                                                                    </Typography>
                                                                </MenuItem>
                                                            </MenuList>
                                                        </Menu>
                                                    </div>
                                                    <Typography color="gray">
                                                        Resolution {config.resolution.w} x {config.resolution.h}
                                                    </Typography>
                                                    <Typography color="gray">
                                                        {config.isFullscreen ? 'Fullscreen' : 'Windowed'}
                                                    </Typography>
                                                    {config.showSplashScreen && (
                                                        <Typography color="gray">Splash Screen Disabled</Typography>
                                                    )}
                                                    {config.mods.map((mod) => {
                                                        return (
                                                            <div
                                                                className="group mt-4 inline-flex flex-wrap items-center gap-3"
                                                                key={mod.name}
                                                            >
                                                                <Tooltip content={'Version ' + mod.version}>
                                                                    <span className="rounded-full border border-green-500/5 bg-green-500/5 p-3 text-green-500 transition-colors hover:border-green-500/10 hover:bg-green-500/10 hover:!opacity-100 group-hover:opacity-70">
                                                                        {mod.name}
                                                                    </span>
                                                                </Tooltip>
                                                            </div>
                                                        );
                                                    })}
                                                </CardBody>
                                                <CardFooter className="pt-3">
                                                    <Button disabled size="lg" fullWidth={true}>
                                                        Launch
                                                    </Button>
                                                </CardFooter>
                                            </Card>
                                        </div>
                                    );
                                })}
                            </div>
                        </TabPanel>
                        <Dialog size="lg" open={deleteDialog} handler={onChangeDelete}>
                            <DialogHeader className="justify-between">
                                <Typography variant="h5" color="blue-gray">
                                    Delete Config
                                </Typography>
                                <IconButton
                                    color="blue-gray"
                                    size="sm"
                                    variant="text"
                                    onClick={onClickCloseDeleteDialog}
                                >
                                    <XMarkIcon strokeWidth={2} className="h-5 w-5" />
                                </IconButton>
                            </DialogHeader>
                            <DialogBody divider>
                                <Typography>The configuration "NeKz's Debug Launch Config" will be deleted.</Typography>
                                <Typography>This action cannot be undone.</Typography>
                                <br />
                                <Typography>Please type "NeKz's Debug Launch Config" to confirm.</Typography>
                                <Input
                                    value={configToDelete}
                                    onChange={onChangeConfigToDelete}
                                    className="mt-2 !border-t-blue-gray-200 focus:!border-t-blue-500"
                                    labelProps={{
                                        className: 'before:content-none after:content-none',
                                    }}
                                    containerProps={{
                                        className: 'min-w-0',
                                    }}
                                    onKeyDown={onKeyDownConfigToDelete}
                                />
                            </DialogBody>
                            <DialogFooter>
                                <Button
                                    size="sm"
                                    variant="filled"
                                    color="red"
                                    disabled={!canConfirmDeletion}
                                    onClick={onClickDeleteConfig}
                                >
                                    <span>Confirm Deletion</span>
                                </Button>
                            </DialogFooter>
                        </Dialog>
                        <TabPanel key="console" value="console">
                            <div className="pb-2">
                                <Textarea
                                    className="!border-t-blue-gray-200 focus:!border-t-blue-500"
                                    labelProps={{
                                        className: 'before:content-none after:content-none',
                                    }}
                                    readOnly
                                />
                            </div>
                            <div className="relative flex w-full">
                                <Input
                                    value={command}
                                    onChange={onChangeCommand}
                                    className="!border-t-blue-gray-200 focus:!border-t-blue-500"
                                    labelProps={{
                                        className: 'before:content-none after:content-none',
                                    }}
                                    containerProps={{
                                        className: 'min-w-0',
                                    }}
                                    onKeyDown={onKeyDownCommand}
                                />
                                <Button
                                    size="sm"
                                    color={command ? 'blue' : 'blue-gray'}
                                    disabled={!command}
                                    className="!absolute right-1 top-1 rounded"
                                    onClick={onClickExecute}
                                >
                                    Execute
                                </Button>
                            </div>
                        </TabPanel>
                        <TabPanel key="settings" value="settings">
                            <div className="flex flex-col w-72 gap-6">
                                <Select
                                    variant="static"
                                    label="Check for updates"
                                    value={checkForUpdates}
                                    onChange={onChangeCheckForUpdates}
                                >
                                    <Option value="disabled">Disabled</Option>
                                    <Option value="on-launcher-start">On launcher start</Option>
                                    <Option value="on-launcher-exit">On launcher exit</Option>
                                </Select>
                            </div>
                            <Typography>TEM Launcher pre-0.1.0</Typography>
                        </TabPanel>
                    </TabsBody>
                </Tabs>
            </main>
            <footer className="fixed bottom-0 w-full bg-white p-8">
                <div className="flex flex-row flex-wrap items-center justify-center gap-y-6 gap-x-12 bg-white text-center md:justify-between">
                    <div></div>
                    <ul className="flex flex-wrap items-center gap-y-2 gap-x-8">
                        <li>
                            <Typography
                                as="a"
                                href="https://tem.nekz.me"
                                target="_blank"
                                rel="noopener noreferrer"
                                color="blue-gray"
                                className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                            >
                                Documentation
                            </Typography>
                        </li>
                        <li>
                            <Typography
                                as="a"
                                href="https://github.com/NeKzor/tem"
                                target="_blank"
                                rel="noopener noreferrer"
                                color="blue-gray"
                                className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                            >
                                Repository
                            </Typography>
                        </li>
                        <li>
                            <Typography
                                as="a"
                                href="https://github.com/NeKzor/tem/issues"
                                target="_blank"
                                rel="noopener noreferrer"
                                color="blue-gray"
                                className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                            >
                                Issues
                            </Typography>
                        </li>
                        <li>
                            <Typography
                                as="a"
                                href="https://github.com/NeKzor/tem/discussions"
                                target="_blank"
                                rel="noopener noreferrer"
                                color="blue-gray"
                                className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                            >
                                Discussions
                            </Typography>
                        </li>
                        <li>
                            <Typography
                                as="a"
                                href="https://github.com/users/NeKzor/projects/1/views/1"
                                target="_blank"
                                rel="noopener noreferrer"
                                color="blue-gray"
                                className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                            >
                                Project
                            </Typography>
                        </li>
                    </ul>
                </div>
                <hr className="my-8 border-blue-gray-50" />
                <Typography color="blue-gray" className="text-center font-normal">
                    &copy; 2023 TEM
                </Typography>
            </footer>
        </div>
    );
}

export default App;
