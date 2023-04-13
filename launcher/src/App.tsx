/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

import { createElement, useCallback, useEffect, useRef, useState } from 'react';
import { invoke } from '@tauri-apps/api/tauri';
import { listen } from '@tauri-apps/api/event';
import {
    Button,
    Card,
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
    Switch,
} from '@material-tailwind/react';
import {
    Bars3Icon,
    PlusIcon,
    CommandLineIcon,
    PencilIcon,
    WrenchIcon,
    RocketLaunchIcon,
    TrashIcon,
    XMarkIcon,
} from '@heroicons/react/24/solid';
import { readTextFile, writeTextFile, BaseDirectory } from '@tauri-apps/api/fs';
import { event } from '@tauri-apps/api';

const configFile = 'config.json';
const maxConfigsAllowed = 4;

interface LauncherConfig {
    name: string;
    createdAt: number;
    modifiedAt: number;
    windowWidth: number;
    windowHeight: number;
    isFullscreen: boolean;
    disableSplashScreen: boolean;
    isDefault: boolean;
    useTEM: boolean;
    useXDead: boolean;
}

const createLauncherConfig = (data?: Partial<LauncherConfig>): LauncherConfig => {
    return Object.assign<LauncherConfig, Partial<LauncherConfig> | undefined>(
        {
            name: '',
            createdAt: 0,
            modifiedAt: 0,
            windowWidth: window.screen.width,
            windowHeight: window.screen.height,
            isFullscreen: true,
            disableSplashScreen: true,
            isDefault: false,
            useTEM: true,
            useXDead: false,
        },
        data,
    );
};

interface LauncherMod {
    name: string;
    version: string;
}

interface LauncherMods {
    tem: LauncherMod;
    xdead: LauncherMod;
}

const defaultMods: LauncherMods = {
    tem: {
        name: 'TEM',
        version: 'pre-0.1.0-20230401',
    },
    xdead: {
        name: 'XDead',
        version: '0.1.0',
    },
};

type SortOption = {
    key: keyof LauncherConfig;
    direction: 'asc' | 'desc';
};

type SortOrderOption = 'createdAt-asc' | 'createdAt-desc' | 'name-asc' | 'name-desc';
type CheckForUpdatesOption = 'disabled' | 'on-launcher-start' | 'on-launcher-exit';

interface AppConfig {
    configs: LauncherConfig[];
    sort: SortOption;
    checkForUpdates: CheckForUpdatesOption;
}

const getSorter =
    ({ key, direction }: SortOption) =>
    (a: LauncherConfig, b: LauncherConfig) => {
        if (key === 'createdAt') {
            if (a.createdAt !== b.createdAt) {
                if (direction === 'asc') {
                    return a.createdAt < b.createdAt ? -1 : 1;
                }
                return a.createdAt < b.createdAt ? 1 : -1;
            }
        } else if (a[key] !== b[key]) {
            return direction === 'asc'
                ? a[key].toString().localeCompare(b[key].toString())
                : b[key].toString().localeCompare(a[key].toString());
        }
        return 0;
    };

const loadConfig = async () => {
    return await readTextFile(configFile, { dir: BaseDirectory.AppLocalData })
        .then((text) => JSON.parse(text) as AppConfig)
        .catch((err) => {
            console.log(`failed to read ${configFile}`, err);
        });
};

const saveConfig = async (config: AppConfig) => {
    console.log('saving config', config);
    await writeTextFile(configFile, JSON.stringify(config), { dir: BaseDirectory.AppLocalData })
        .then(() => {
            console.log('saved config');
        })
        .catch((err) => {
            console.log(`failed to write ${configFile}`, err);
        });
};

function App() {
    const [consoleBuffer, setConsoleBuffer] = useState<string[]>([]);
    const [command, setCommand] = useState('');
    const [checkForUpdates, setCheckForUpdates] = useState<CheckForUpdatesOption>('disabled');
    const [configs, setConfigs] = useState<LauncherConfig[]>([]);
    const [mods, setMods] = useState<LauncherMods>(defaultMods);
    const [config, setConfig] = useState<LauncherConfig>(createLauncherConfig);
    const [editDialog, setEditDialog] = useState(false);
    const [configToDelete, setConfigToDelete] = useState<LauncherConfig | undefined>(undefined);
    const [configNameToDelete, setConfigNameToDelete] = useState('');
    const [deleteDialog, setDeleteDialog] = useState(false);
    const [sort, setSort] = useState<SortOption>({ key: 'createdAt', direction: 'asc' });
    const [sortOrder, setSortOrder] = useState<SortOrderOption>('createdAt-asc');
    const [shouldSaveConfig, setShouldSaveConfig] = useState(false);
    const [gameLaunched, setGameLaunched] = useState(false);
    const nameRef = useRef<HTMLInputElement>(null);

    const onClickLaunch = (config: LauncherConfig) => {
        setGameLaunched(true);

        invoke('launch_config', { config })
            .catch((err) => {
                console.error(err);
            })
            .finally(() => {
                setGameLaunched(false);
            });
    };

    useEffect(() => {
        if (shouldSaveConfig) {
            console.log('saving config');
            setShouldSaveConfig(false);
            saveConfig({ configs, sort, checkForUpdates });
        }
    }, [shouldSaveConfig]);

    const onChangeSortOrder = useCallback(
        (value?: string) => {
            const [key, direction] = value?.split('-') ?? '';
            const sort = { key, direction } as SortOption;
            setSort(sort);
            setSortOrder(value as SortOrderOption);
            setConfigs((configs) => configs.sort(getSorter(sort)));
            console.log('saving order');
            setShouldSaveConfig(true);
        },
        [setSort, setSortOrder, setConfigs, setShouldSaveConfig],
    );

    const onChangeEdit = useCallback(
        (config?: LauncherConfig) => {
            if (!editDialog) {
                const isFirstConfig = configs.length === 0;
                setConfig(
                    structuredClone(config) ??
                        createLauncherConfig({
                            name: isFirstConfig ? 'Default' : '',
                            isDefault: isFirstConfig,
                        }),
                );
                setEditDialog(true);
                setTimeout(() => {
                    nameRef.current?.querySelector('input')?.focus();
                }, 100);
            }
        },
        [configs, editDialog, setConfig, setEditDialog],
    );

    const isEdit = config.createdAt !== 0;

    const onClickCloseEditDialog = useCallback(() => {
        setEditDialog(false);
    }, [setEditDialog]);

    const onClickEditConfig = useCallback(
        (config: LauncherConfig) => {
            if (!config.createdAt) {
                config.createdAt = new Date().getTime();
                config.modifiedAt = config.createdAt;
            } else {
                config.modifiedAt = new Date().getTime();
            }

            setConfigs((configs) => {
                const unsetDefault = (config: LauncherConfig) => {
                    config.isDefault = false;
                    return config;
                };
                const orderConfigs = (configs: LauncherConfig[]) => {
                    return configs.sort(getSorter(sort));
                };

                const updated = config.isDefault ? configs.map(unsetDefault) : configs;
                const index = configs.findIndex(({ createdAt }) => createdAt === config.createdAt);

                return orderConfigs(
                    index !== -1
                        ? [...updated.slice(0, index), ...updated.slice(index + 1), config]
                        : [...updated, config],
                );
            });

            setEditDialog(false);
            setShouldSaveConfig(true);
        },
        [sort, setConfig, setEditDialog, setShouldSaveConfig],
    );

    const onChangeDelete = useCallback(
        (config: LauncherConfig) => {
            if (!deleteDialog) {
                setConfigToDelete(config);
                setDeleteDialog(true);
            }
        },
        [deleteDialog, setConfigToDelete, setDeleteDialog],
    );

    const onClickCloseDeleteDialog = useCallback(() => {
        setConfigNameToDelete('');
        setDeleteDialog(false);
    }, [setConfigNameToDelete, setDeleteDialog]);

    const onChangeConfigToDelete = useCallback(({ target }: any) => {
        setConfigNameToDelete(target.value as string);
    }, []);

    const onClickDeleteConfig = useCallback(() => {
        const index = configs.findIndex(({ createdAt }) => createdAt === configToDelete?.createdAt);
        if (index !== -1) {
            setConfigs((configs) => [...configs.slice(0, index), ...configs.slice(index + 1)]);
            setShouldSaveConfig(true);
        }

        setConfigNameToDelete('');
        setDeleteDialog(false);
    }, [configToDelete, setConfigNameToDelete, setDeleteDialog, setShouldSaveConfig]);

    const canConfirmDeletion = configNameToDelete === configToDelete?.name;

    const onKeyDownConfigToDelete = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter' && canConfirmDeletion) {
                onClickDeleteConfig();
            }
        },
        [onClickDeleteConfig, canConfirmDeletion],
    );

    useEffect(() => {
        if (!configs.length) {
            loadConfig().then((config) => {
                if (config) {
                    console.log('found config');
                    setSort(config.sort);
                    setSortOrder([config.sort.key, config.sort.direction].join('-') as SortOrderOption);
                    setConfigs(config.configs.map(createLauncherConfig));
                }
            });
        }

        const unlistenGameLaunched = listen('game-launched', (event: event.Event<boolean>) => {
            console.log({ event });
            setGameLaunched(event.payload);
        });

        console.log('initialized');

        return () => {
            unlistenGameLaunched.then((unlisten) => {
                unlisten();
                console.log('uninitialized');
            });
        };
    }, []);

    // useEffect(() => {
    //     invoke('console_buffer')
    //         .then((newConsoleBuffer) => setConsoleBuffer([...consoleBuffer, ...(newConsoleBuffer as string[])]))
    //         .catch((error) => setConsoleBuffer([...consoleBuffer, error as string]));
    // }, [setConsoleBuffer]);

    const onClickExecute = useCallback(() => {
        const text = command;
        invoke('console_execute', { text })
            .then((line) => setConsoleBuffer([...consoleBuffer, `> ${text}`, line as string]))
            .catch((error) => setConsoleBuffer([...consoleBuffer, `> ${text}`, error as string]));
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

    const onChangeCheckForUpdates = useCallback(
        (value: any) => {
            setCheckForUpdates(value);
            console.log('updates save');
            setShouldSaveConfig(true);
        },
        [setCheckForUpdates],
    );

    return (
        <div className="flex flex-col h-screen justify-between">
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
                            <div className="flex flex-row gap-2 mb-6 mt-2">
                                <div>
                                    <Button
                                        className="flex items-center gap-3"
                                        disabled={configs.length === maxConfigsAllowed}
                                        onClick={() => onChangeEdit()}
                                    >
                                        <PlusIcon strokeWidth={2} className="h-5 w-5" />
                                        Create Config
                                    </Button>
                                </div>
                                {configs.length > 1 && (
                                    <div className="absolute right-0 mr-4">
                                        <Select
                                            variant="static"
                                            label="Sort by"
                                            value={sortOrder}
                                            onChange={onChangeSortOrder}
                                        >
                                            <Option value="createdAt-asc">Date (oldest)</Option>
                                            <Option value="createdAt-desc">Date (newest)</Option>
                                            <Option value="name-asc">Name (asc.)</Option>
                                            <Option value="name-desc">Name (desc.)</Option>
                                        </Select>
                                    </div>
                                )}
                            </div>
                            <div className="grid sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-3 xl:grid-cols-4 2xl:grid-cols-5 gap-4 mb-4">
                                {configs.map((config, idx) => {
                                    return (
                                        <div key={idx}>
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
                                                                    onClick={() => onChangeEdit(config)}
                                                                >
                                                                    <PencilIcon strokeWidth={2} className="h-4 w-4" />
                                                                    <Typography variant="small" className="font-normal">
                                                                        Edit
                                                                    </Typography>
                                                                </MenuItem>
                                                                <MenuItem
                                                                    className="flex items-center gap-2 hover:bg-red-500"
                                                                    onClick={() => onChangeDelete(config)}
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
                                                        Resolution {config.windowWidth} x {config.windowHeight}
                                                    </Typography>
                                                    <Typography color="gray">
                                                        {config.isFullscreen ? 'Fullscreen' : 'Windowed'}
                                                    </Typography>
                                                    <Typography color="gray">
                                                        Splash Screen{' '}
                                                        {config.disableSplashScreen ? 'Disabled' : 'Enabled'}
                                                    </Typography>
                                                    {config.useTEM && (
                                                        <div className="group mt-4 inline-flex flex-wrap items-center gap-3">
                                                            <Tooltip content={mods.tem.version}>
                                                                <span className="rounded-full border border-blue-500/5 bg-blue-500/5 p-3 text-blue-500 transition-colors hover:border-blue-500/10 hover:bg-blue-500/10 hover:!opacity-100 group-hover:opacity-70">
                                                                    TEM
                                                                </span>
                                                            </Tooltip>
                                                        </div>
                                                    )}
                                                    {config.useTEM && config.useXDead && (
                                                        <div className="group mt-4 ml-2 inline-flex flex-wrap items-center gap-3">
                                                            <Tooltip content={mods.xdead.version}>
                                                                <span className="rounded-full border border-green-500/5 bg-green-500/5 p-3 text-green-500 transition-colors hover:border-green-500/10 hover:bg-green-500/10 hover:!opacity-100 group-hover:opacity-70">
                                                                    XDead
                                                                </span>
                                                            </Tooltip>
                                                        </div>
                                                    )}
                                                    {!config.useTEM && (
                                                        <div className="group mt-4 inline-flex flex-wrap items-center gap-3">
                                                            <span className="rounded-full border border-gray-500/5 bg-gray-500/5 p-3 text-gray-500 transition-colors hover:border-gray-500/10 hover:bg-gray-500/10 hover:!opacity-100 group-hover:opacity-70">
                                                                No Mods
                                                            </span>
                                                        </div>
                                                    )}
                                                </CardBody>
                                                <CardFooter className="pt-3">
                                                    <Button
                                                        disabled={gameLaunched}
                                                        size="lg"
                                                        fullWidth={true}
                                                        onClick={() => onClickLaunch(config)}
                                                    >
                                                        Launch {config.isDefault ? ' (default)' : ''}
                                                    </Button>
                                                </CardFooter>
                                            </Card>
                                        </div>
                                    );
                                })}
                            </div>
                            <Dialog size="md" open={editDialog} handler={onChangeEdit}>
                                <DialogHeader className="justify-between">
                                    <Typography variant="h5" color="blue-gray">
                                        {isEdit ? 'Edit' : 'Create'} Config
                                    </Typography>
                                    <IconButton
                                        color="blue-gray"
                                        size="sm"
                                        variant="text"
                                        onClick={onClickCloseEditDialog}
                                    >
                                        <XMarkIcon strokeWidth={2} className="h-5 w-5" />
                                    </IconButton>
                                </DialogHeader>
                                <DialogBody className="pt-0" divider>
                                    <Input
                                        ref={nameRef}
                                        className="w-4"
                                        value={config.name}
                                        variant="outlined"
                                        label="Name"
                                        autoFocus={true}
                                        containerProps={{ className: 'mt-4' }}
                                        onChange={({ target: { value } }) =>
                                            setConfig((oldConfig) => ({ ...oldConfig, name: value }))
                                        }
                                    />
                                    <div className="flex items-center gap-4">
                                        <Input
                                            value={config.windowWidth.toString()}
                                            variant="outlined"
                                            label="Window Width"
                                            type="number"
                                            containerProps={{ className: 'mt-4' }}
                                            onChange={({ target: { value } }) =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    windowWidth: parseInt(value, 10),
                                                }))
                                            }
                                        />
                                        <Input
                                            value={config.windowHeight.toString()}
                                            variant="outlined"
                                            label="Window Height"
                                            type="number"
                                            containerProps={{ className: 'mt-4' }}
                                            onChange={({ target: { value } }) =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    windowHeight: parseInt(value, 10),
                                                }))
                                            }
                                        />
                                    </div>
                                    <div className="mt-4">
                                        <Switch
                                            id="fullscreen"
                                            label="Fullscreen"
                                            defaultChecked={config.isFullscreen}
                                            onChange={() =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    isFullscreen: !oldConfig.isFullscreen,
                                                }))
                                            }
                                        />
                                    </div>
                                    <div className="mt-4">
                                        <Switch
                                            id="splash-screen"
                                            label="Disable splash screen"
                                            defaultChecked={config.disableSplashScreen}
                                            onChange={() =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    disableSplashScreen: !oldConfig.disableSplashScreen,
                                                }))
                                            }
                                        />
                                    </div>
                                    <div className="mt-4">
                                        <Switch
                                            id="tem"
                                            label="Use TEM"
                                            defaultChecked={config.useTEM}
                                            onChange={() =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    useTEM: !oldConfig.useTEM,
                                                }))
                                            }
                                        />
                                    </div>
                                    <div className="mt-4">
                                        <Switch
                                            id="xdead"
                                            label="Use XDead"
                                            defaultChecked={config.useXDead}
                                            onChange={() =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    useXDead: !oldConfig.useXDead,
                                                }))
                                            }
                                        />
                                    </div>
                                    <div className="mt-4">
                                        <Switch
                                            id="default"
                                            label="Use this as default"
                                            defaultChecked={config.isDefault}
                                            onChange={() =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    isDefault: !oldConfig.isDefault,
                                                }))
                                            }
                                        />
                                    </div>
                                </DialogBody>
                                <DialogFooter>
                                    <Button
                                        size="sm"
                                        variant="filled"
                                        disabled={!config.name}
                                        onClick={() => onClickEditConfig(config)}
                                    >
                                        <span>Save</span>
                                    </Button>
                                </DialogFooter>
                            </Dialog>
                        </TabPanel>
                        <Dialog size="md" open={deleteDialog} handler={onChangeDelete}>
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
                                <Typography>The configuration "{configToDelete?.name}" will be deleted.</Typography>
                                <Typography>This action cannot be undone.</Typography>
                                <br />
                                <Typography>Please type "{configToDelete?.name}" to confirm.</Typography>
                                <Input
                                    value={configNameToDelete}
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
                            <div className="flex pb-2">
                                <Textarea
                                    value={consoleBuffer.join('\n')}
                                    className="!border-t-blue-gray-200 focus:!border-t-blue-500 h-[60vh]"
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
                            <div className="flex flex-col w-72 gap-6 mt-4 min-h-[200px]">
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
                        </TabPanel>
                    </TabsBody>
                </Tabs>
            </main>
            <footer className="fixed bottom-0 w-full bg-white p-8 pt-0 z-50">
                <hr className="mb-8 border-blue-gray-50" />
                <div className="flex flex-row flex-wrap items-center justify-center gap-y-6 gap-x-12 bg-white text-center md:justify-between">
                    <Typography color="blue-gray" className="text-center font-normal">
                        &copy; 2023 TEM
                    </Typography>
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
            </footer>
        </div>
    );
}

export default App;
