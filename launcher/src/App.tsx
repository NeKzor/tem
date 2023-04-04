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

interface LauncherConfig {
    name: string;
    createdAt: number;
    modifiedAt: number;
    resolution: {
        width: number;
        height: number;
    };
    isFullscreen: boolean;
    disableSplashScreen: boolean;
    isDefault: boolean;
    useTEM: boolean;
    useXDead: boolean;
}

interface LauncherMod {
    name: string;
    version: string;
}

interface LauncherMods {
    tem: LauncherMod;
    xdead: LauncherMod;
}

const defaultConfig: LauncherConfig = {
    name: '',
    createdAt: 0,
    modifiedAt: 0,
    resolution: {
        width: window.screen.width,
        height: window.screen.height,
    },
    isFullscreen: true,
    disableSplashScreen: true,
    isDefault: false,
    useTEM: true,
    useXDead: false,
};

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

interface AppConfig {
    configs: LauncherConfig[];
    sort: SortOption;
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
    return await readTextFile('config.json', { dir: BaseDirectory.AppConfig })
        .then((text) => JSON.parse(text) as AppConfig)
        .catch(() => {
            console.log('failed to read config.json');
        });
};

const saveConfig = async (config: AppConfig) => {
    await writeTextFile('config.json', JSON.stringify(config), { dir: BaseDirectory.AppConfig })
        .then((result) => {
            console.log('result', result);
        })
        .catch(() => {
            console.log('failed to write config.json');
        });
};

function App() {
    const [consoleBuffer, setConsoleBuffer] = useState<string[]>([]);
    const [command, setCommand] = useState('');
    const [checkForUpdates, setCheckForUpdates] = useState('disabled');
    const [configs, setConfigs] = useState<LauncherConfig[]>([]);
    const [mods, setMods] = useState<LauncherMods>(defaultMods);
    const [config, setConfig] = useState<LauncherConfig>(defaultConfig);
    const [editDialog, setEditDialog] = useState(false);
    const [configToDelete, setConfigToDelete] = useState<LauncherConfig | undefined>(undefined);
    const [configNameToDelete, setConfigNameToDelete] = useState('');
    const [deleteDialog, setDeleteDialog] = useState(false);
    const [sort, setSort] = useState<SortOption>({ key: 'createdAt', direction: 'asc' });
    const [sortOrder, setSortOrder] = useState<SortOrderOption>('createdAt-asc');

    const onChangeSortOrder = useCallback(
        (value?: string) => {
            const [key, direction] = value?.split('-') ?? '';
            const sort = { key, direction } as SortOption;
            setSort(sort);
            setSortOrder(value as SortOrderOption);
            setConfigs(configs.sort(getSorter(sort)));
        },
        [configs, setSort, setSortOrder, setConfigs],
    );

    const onChangeEdit = useCallback(
        (config?: LauncherConfig) => {
            if (!editDialog) {
                setConfig(
                    JSON.parse(
                        JSON.stringify(
                            config ?? ({ ...defaultConfig, isDefault: configs.length === 0 } as LauncherConfig),
                        ),
                    ),
                );
                setEditDialog(true);
                // TODO: focus first input element
            }
        },
        [configs, editDialog, setConfig, setEditDialog],
    );

    const isEdit = useMemo(() => config.createdAt !== 0, [config]);

    const onClickCloseEditDialog = useCallback(() => {
        setEditDialog(false);
    }, [setEditDialog]);

    const onClickEditConfig = useCallback(() => {
        if (!config.createdAt) {
            config.createdAt = new Date().getTime();
            config.modifiedAt = config.createdAt;
        } else {
            config.modifiedAt = new Date().getTime();
        }

        const orderConfigs = (configs: LauncherConfig[]) => configs.sort(getSorter(sort));
        const updateDefault = (config: LauncherConfig) => {
            config.isDefault = false;
            return config;
        };

        const updated = config.isDefault ? configs.map(updateDefault) : configs;
        const index = configs.findIndex(({ createdAt }) => createdAt === config.createdAt);

        if (index !== -1) {
            setConfigs(orderConfigs([...updated.slice(0, index), ...updated.slice(index + 1), config]));
        } else {
            setConfigs(orderConfigs([...updated, config]));
        }

        setEditDialog(false);
        saveConfig({ configs, sort });
    }, [sort, config, configs, setConfig, setEditDialog]);

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
            setConfigs([...configs.slice(0, index), ...configs.slice(index + 1)]);
        }

        saveConfig({ configs, sort });
        setConfigNameToDelete('');
        setDeleteDialog(false);
    }, [configToDelete, setConfigNameToDelete, setDeleteDialog]);

    const canConfirmDeletion = useMemo(() => {
        return configNameToDelete === configToDelete?.name;
    }, [configToDelete, configNameToDelete]);

    const onKeyDownConfigToDelete = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter' && canConfirmDeletion) {
                onClickDeleteConfig();
            }
        },
        [onClickDeleteConfig, canConfirmDeletion],
    );

    useEffect(() => {
        loadConfig().then((config) => {
            if (config) {
                setSort(config.sort);
                setSortOrder([config.sort.key, config.sort.direction].join('-') as SortOrderOption);
                setConfigs(config.configs);
            }
        });
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
                            <div className="flex items-center gap-2 mb-6 mt-2">
                                <div>
                                    <Button className="flex items-center gap-3" onClick={() => onChangeEdit()}>
                                        <PlusIcon strokeWidth={2} className="h-5 w-5" />
                                        Create Config
                                    </Button>
                                </div>
                                {configs.length > 1 && (
                                    <div className="ml-4">
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
                                                        Resolution {config.resolution.width} x{' '}
                                                        {config.resolution.height}
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
                                                    <Button disabled size="lg" fullWidth={true}>
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
                                <DialogBody divider>
                                    <Input
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
                                            value={config.resolution.width.toString()}
                                            variant="outlined"
                                            label="Window Width"
                                            type="number"
                                            containerProps={{ className: 'mt-4' }}
                                            onChange={({ target: { value } }) =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    resolution: {
                                                        ...oldConfig.resolution,
                                                        width: parseInt(value, 10),
                                                    },
                                                }))
                                            }
                                        />
                                        <Input
                                            value={config.resolution.height.toString()}
                                            variant="outlined"
                                            label="Window Height"
                                            type="number"
                                            containerProps={{ className: 'mt-4' }}
                                            onChange={({ target: { value } }) =>
                                                setConfig((oldConfig) => ({
                                                    ...oldConfig,
                                                    resolution: {
                                                        ...oldConfig.resolution,
                                                        height: parseInt(value, 10),
                                                    },
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
                                        onClick={onClickEditConfig}
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
                <hr className="my-8 border-blue-gray-50" />
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
