import { useCallback, useContext, useEffect, useRef, useState } from 'react';
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
import { Bars3Icon, PlusIcon, PencilIcon, TrashIcon, XMarkIcon } from '@heroicons/react/24/solid';
import { readTextFile, BaseDirectory } from '@tauri-apps/api/fs';
import { event } from '@tauri-apps/api';
import { LauncherConfig, LauncherMods, SortOption, SortOrderOption, createLauncherConfig, defaultMods } from '../Types';
import AppState, { DispatchAction } from '../AppState';

const maxConfigsAllowed = 4;

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

function Launcher({
    gameLaunched,
    onGameLaunched,
}: {
    gameLaunched: boolean;
    onGameLaunched: (value: boolean) => void;
}) {
    const {
        state: { configs, sort },
        dispatch,
    } = useContext(AppState);

    const [mods, setMods] = useState<LauncherMods>(defaultMods);
    const [config, setConfig] = useState<LauncherConfig>(createLauncherConfig);
    const [editDialog, setEditDialog] = useState(false);
    const [configToDelete, setConfigToDelete] = useState<LauncherConfig | undefined>(undefined);
    const [configNameToDelete, setConfigNameToDelete] = useState('');
    const [deleteDialog, setDeleteDialog] = useState(false);
    const [sortOrder, setSortOrder] = useState<SortOrderOption>('createdAt-asc');
    const nameRef = useRef<HTMLInputElement>(null);
    const isEdit = config.createdAt !== 0;

    useEffect(() => {
        setSortOrder([sort.key, sort.direction].join('-') as SortOrderOption);
    }, [sort, setSortOrder]);

    useEffect(() => {
        const toVersion = (release: string) => {
            release = release.replace(/^(tem|xdead)\-/, '').replace(/\.zip$/, '');
            return release ? release : 'unknown version';
        };

        // TODO: update once we downloaded a new version
        readTextFile('mods/tem/release.txt', { dir: BaseDirectory.Resource })
            .then((release) => setMods((mods) => ({ ...mods, tem: { ...mods.tem, version: toVersion(release) } })))
            .catch(console.error);

        readTextFile('mods/xdead/release.txt', { dir: BaseDirectory.Resource })
            .then((release) => setMods((mods) => ({ ...mods, xdead: { ...mods.xdead, version: toVersion(release) } })))
            .catch(console.error);

        const unlistenGameLaunched = listen('game-launched', (event: event.Event<boolean>) => {
            console.log({ event });
            onGameLaunched(event.payload);
        });

        console.log('initialized');

        return () => {
            Promise.all([unlistenGameLaunched]).then((unlistenAll) => {
                unlistenAll.forEach((unlisten) => unlisten());
                console.log('uninitialized');
            });
        };
    }, []);

    const onClickLaunch = (config: LauncherConfig) => {
        onGameLaunched(true);
        dispatch(DispatchAction.LaunchConfig({ config, onComplete: () => onGameLaunched(false) }));
    };

    const onChangeSortOrder = useCallback(
        (value?: string) => {
            setSortOrder(value as SortOrderOption);

            const [key, direction] = value?.split('-') ?? '';
            const sort = { key, direction } as SortOption;

            dispatch(DispatchAction.SetSort(sort));
            dispatch(DispatchAction.SetConfigs((configs) => configs.sort(getSorter(sort))));
            dispatch(DispatchAction.SaveConfig());
        },
        [dispatch, setSortOrder],
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

            dispatch(
                DispatchAction.SetConfigs((configs) => {
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
                }),
            );

            setEditDialog(false);
            dispatch(DispatchAction.SaveConfig());
        },
        [sort, configs, setConfig, setEditDialog],
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
            dispatch(DispatchAction.SetConfigs((configs) => [...configs.slice(0, index), ...configs.slice(index + 1)]));
            dispatch(DispatchAction.SaveConfig());
        }

        setConfigNameToDelete('');
        setDeleteDialog(false);
    }, [configToDelete, setConfigNameToDelete, setDeleteDialog]);

    const canConfirmDeletion = configNameToDelete === configToDelete?.name;

    const onKeyDownConfigToDelete = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter' && canConfirmDeletion) {
                onClickDeleteConfig();
            }
        },
        [onClickDeleteConfig, canConfirmDeletion],
    );

    return (
        <>
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
                        <Select variant="static" label="Sort by" value={sortOrder} onChange={onChangeSortOrder}>
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
                                        <Typography variant="h5" color="blue-gray" className="font-medium">
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
                                        Splash Screen {config.disableSplashScreen ? 'Disabled' : 'Enabled'}
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
                    <IconButton color="blue-gray" size="sm" variant="text" onClick={onClickCloseEditDialog}>
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
                        onChange={({ target: { value } }) => setConfig((oldConfig) => ({ ...oldConfig, name: value }))}
                    />
                    <div className="flex items-center gap-4">
                        <Input
                            value={config.windowWidth.toString()}
                            variant="outlined"
                            label="Window Width"
                            type="number"
                            containerProps={{ className: 'mt-4' }}
                            onChange={({ target: { valueAsNumber } }) =>
                                setConfig((oldConfig) => ({
                                    ...oldConfig,
                                    windowWidth: valueAsNumber,
                                }))
                            }
                        />
                        <Input
                            value={config.windowHeight.toString()}
                            variant="outlined"
                            label="Window Height"
                            type="number"
                            containerProps={{ className: 'mt-4' }}
                            onChange={({ target: { valueAsNumber } }) =>
                                setConfig((oldConfig) => ({
                                    ...oldConfig,
                                    windowHeight: valueAsNumber,
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
            <Dialog size="md" open={deleteDialog} handler={onChangeDelete}>
                <DialogHeader className="justify-between">
                    <Typography variant="h5" color="blue-gray">
                        Delete Config
                    </Typography>
                    <IconButton color="blue-gray" size="sm" variant="text" onClick={onClickCloseDeleteDialog}>
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
        </>
    );
}

export default Launcher;
