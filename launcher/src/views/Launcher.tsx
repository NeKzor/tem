import { useCallback, useContext, useEffect, useRef, useState } from 'react';
import { listen } from '@tauri-apps/api/event';
import { Button, Select, Option } from '@material-tailwind/react';
import { PlusIcon } from '@heroicons/react/24/solid';
import { readTextFile, BaseDirectory } from '@tauri-apps/api/fs';
import { event } from '@tauri-apps/api';
import { LauncherConfig, LauncherMods, SortOption, SortOrderOption, createLauncherConfig, defaultMods } from '../Types';
import AppState, { DispatchAction } from '../AppState';
import ConfigCard from '../components/ConfigCard';
import ConfigEditDialog from '../components/ConfigEditDialog';
import ConfigDeleteDialog from '../components/ConfigDeleteDialog';

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
    const [configToEdit, setConfigToEdit] = useState(createLauncherConfig);
    const [editDialog, setEditDialog] = useState(false);
    const [configToDelete, setConfigToDelete] = useState<LauncherConfig | undefined>(undefined);
    const [deleteDialog, setDeleteDialog] = useState(false);
    const [sortOrder, setSortOrder] = useState<SortOrderOption>('createdAt-asc');

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
                setConfigToEdit(
                    structuredClone(config) ??
                        createLauncherConfig({
                            name: isFirstConfig ? 'Default' : '',
                            isDefault: isFirstConfig,
                        }),
                );
                setEditDialog(true);
            }
        },
        [configs, editDialog, setConfigToEdit, setEditDialog],
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
        [sort, configs, setConfigToEdit, setEditDialog],
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
        setDeleteDialog(false);
    }, [setDeleteDialog]);

    const onClickDeleteConfig = useCallback(() => {
        const index = configs.findIndex(({ createdAt }) => createdAt === configToDelete?.createdAt);
        if (index !== -1) {
            dispatch(DispatchAction.SetConfigs((configs) => [...configs.slice(0, index), ...configs.slice(index + 1)]));
            dispatch(DispatchAction.SaveConfig());
        }

        setDeleteDialog(false);
    }, [configToDelete, setDeleteDialog]);

    const onClickCreateConfig = useCallback(() => onChangeEdit(), [onChangeEdit]);

    return (
        <>
            <div className="flex flex-row gap-2 mb-6 mt-2">
                <div>
                    <Button
                        className="flex items-center gap-3"
                        disabled={configs.length === maxConfigsAllowed}
                        onClick={onClickCreateConfig}
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
                            <ConfigCard
                                config={config}
                                mods={mods}
                                gameLaunched={gameLaunched}
                                onClickLaunch={onClickLaunch}
                                onChangeEdit={onChangeEdit}
                                onChangeDelete={onChangeDelete}
                            />
                        </div>
                    );
                })}
            </div>
            <ConfigEditDialog
                editDialog={editDialog}
                configToEdit={configToEdit}
                onChangeEdit={onChangeEdit}
                onClickEditConfig={onClickEditConfig}
                onClickCloseEditDialog={onClickCloseEditDialog}
            />
            <ConfigDeleteDialog
                deleteDialog={deleteDialog}
                configToDelete={configToDelete}
                onChangeDelete={onChangeDelete}
                onClickDeleteConfig={onClickDeleteConfig}
                onClickCloseDeleteDialog={onClickCloseDeleteDialog}
            />
        </>
    );
}

export default Launcher;
