import { Bars3Icon, PencilIcon, TrashIcon } from '@heroicons/react/24/solid';
import {
    Button,
    Card,
    CardBody,
    CardFooter,
    IconButton,
    Menu,
    MenuHandler,
    MenuItem,
    MenuList,
    Tooltip,
    Typography,
} from '@material-tailwind/react';
import { LauncherConfig, LauncherMods } from '../Types';
import { useCallback } from 'react';

type ConfigCardProps = {
    config: LauncherConfig;
    mods: LauncherMods;
    gameLaunched: boolean;
    onClickLaunch: (config: LauncherConfig) => void;
    onChangeEdit: (config: LauncherConfig) => void;
    onChangeDelete: (config: LauncherConfig) => void;
};

function ConfigCard({ config, mods, gameLaunched, onClickLaunch, onChangeEdit, onChangeDelete }: ConfigCardProps) {
    const onClickLaunchConfig = useCallback(() => onClickLaunch(config), [onClickLaunch, config]);
    const onClickEditConfig = useCallback(() => onChangeEdit(config), [onChangeEdit, config]);
    const onClickDeleteConfig = useCallback(() => onChangeDelete(config), [onChangeDelete, config]);

    return (
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
                            <MenuItem className="flex items-center gap-2" onClick={onClickEditConfig}>
                                <PencilIcon strokeWidth={2} className="h-4 w-4" />
                                <Typography variant="small" className="font-normal">
                                    Edit
                                </Typography>
                            </MenuItem>
                            <MenuItem
                                className="flex items-center gap-2 hover:bg-red-500"
                                onClick={onClickDeleteConfig}
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
                <Typography color="gray">{config.isFullscreen ? 'Fullscreen' : 'Windowed'}</Typography>
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
                <Button disabled={gameLaunched} size="lg" fullWidth={true} onClick={onClickLaunchConfig}>
                    Launch {config.isDefault ? ' (default)' : ''}
                </Button>
            </CardFooter>
        </Card>
    );
}

export default ConfigCard;
