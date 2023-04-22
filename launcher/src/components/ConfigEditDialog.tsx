import { XMarkIcon } from '@heroicons/react/24/solid';
import {
    Button,
    Dialog,
    DialogBody,
    DialogFooter,
    DialogHeader,
    IconButton,
    Input,
    Switch,
    Typography,
} from '@material-tailwind/react';
import { LauncherConfig, createLauncherConfig } from '../Types';
import { ChangeEvent, useCallback, useEffect, useRef, useState } from 'react';

type ConfigEditDialogProps = {
    editDialog: boolean;
    configToEdit: LauncherConfig;
    onChangeEdit: (config?: LauncherConfig) => void;
    onClickEditConfig: (config: LauncherConfig) => void;
    onClickCloseEditDialog: () => void;
};

function ConfigEditDialog({
    editDialog,
    configToEdit,
    onChangeEdit,
    onClickEditConfig,
    onClickCloseEditDialog,
}: ConfigEditDialogProps) {
    const [config, setConfig] = useState(createLauncherConfig);
    const nameRef = useRef<HTMLInputElement>(null);

    const isEdit = config.createdAt !== 0;

    useEffect(() => {
        setConfig(configToEdit);
    }, [setConfig, configToEdit]);

    useEffect(() => {
        if (editDialog) {

            setTimeout(() => {
                nameRef.current?.querySelector('input')?.focus();
            }, 100);
        }
    }, [editDialog]);

    const onChangeName = useCallback(({ target: { value } }: ChangeEvent<HTMLInputElement>) => {
        setConfig((oldConfig) => ({ ...oldConfig, name: value }));
    }, [setConfig]);
    const onChangeNameWindowWidth = useCallback(({ target: { valueAsNumber } }: ChangeEvent<HTMLInputElement>) => {
        setConfig((oldConfig) => ({ ...oldConfig, windowWidth: valueAsNumber }));
    }, [setConfig]);
    const onChangeNameWindowHeight = useCallback(({ target: { valueAsNumber } }: ChangeEvent<HTMLInputElement>) => {
        setConfig((oldConfig) => ({ ...oldConfig, windowHeight: valueAsNumber }));
    }, [setConfig]);
    const onChangeIsFullscreen = useCallback(() => {
        setConfig((oldConfig) => ({ ...oldConfig, isFullscreen: !oldConfig.isFullscreen }));
    }, [setConfig]);
    const onChangeDisableSplashScreen = useCallback(() => {
        setConfig((oldConfig) => ({ ...oldConfig, disableSplashScreen: !oldConfig.disableSplashScreen }));
    }, [setConfig]);
    const onChangeUseTEM = useCallback(() => {
        setConfig((oldConfig) => ({ ...oldConfig, useTEM: !oldConfig.useTEM }));
    }, [setConfig]);
    const onChangeUseXDead = useCallback(() => {
        setConfig((oldConfig) => ({ ...oldConfig, useXDead: !oldConfig.useXDead }));
    }, [setConfig]);
    const onChangeIsDefault = useCallback(() => {
        setConfig((oldConfig) => ({ ...oldConfig, isDefault: !oldConfig.isDefault }));
    }, [setConfig]);

    const onClickSave = useCallback(() => onClickEditConfig(config), [onClickEditConfig, config]);

    return (
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
                    onChange={onChangeName}
                />
                <div className="flex items-center gap-4">
                    <Input
                        value={config.windowWidth.toString()}
                        variant="outlined"
                        label="Window Width"
                        type="number"
                        containerProps={{ className: 'mt-4' }}
                        onChange={onChangeNameWindowWidth}
                    />
                    <Input
                        value={config.windowHeight.toString()}
                        variant="outlined"
                        label="Window Height"
                        type="number"
                        containerProps={{ className: 'mt-4' }}
                        onChange={onChangeNameWindowHeight}
                    />
                </div>
                <div className="mt-4">
                    <Switch
                        id="fullscreen"
                        label="Fullscreen"
                        checked={config.isFullscreen}
                        onChange={onChangeIsFullscreen}
                    />
                </div>
                <div className="mt-4">
                    <Switch
                        id="splash-screen"
                        label="Disable splash screen"
                        checked={config.disableSplashScreen}
                        onChange={onChangeDisableSplashScreen}
                    />
                </div>
                <div className="mt-4">
                    <Switch id="tem" label="Use TEM" checked={config.useTEM} onChange={onChangeUseTEM} />
                </div>
                <div className="mt-4">
                    <Switch id="xdead" label="Use XDead" checked={config.useXDead} onChange={onChangeUseXDead} />
                </div>
                <div className="mt-4">
                    <Switch
                        id="default"
                        label="Use this as default"
                        checked={config.isDefault}
                        onChange={onChangeIsDefault}
                    />
                </div>
            </DialogBody>
            <DialogFooter>
                <Button size="sm" variant="filled" disabled={!config.name} onClick={onClickSave}>
                    <span>Save</span>
                </Button>
            </DialogFooter>
        </Dialog>
    );
}

export default ConfigEditDialog;
