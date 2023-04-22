import { XMarkIcon } from '@heroicons/react/24/solid';
import {
    Button,
    Dialog,
    DialogBody,
    DialogFooter,
    DialogHeader,
    IconButton,
    Input,
    Typography,
} from '@material-tailwind/react';
import { LauncherConfig } from '../Types';
import { useCallback, useEffect, useState } from 'react';

type ConfigDeleteDialogProps = {
    deleteDialog: boolean;
    configToDelete: LauncherConfig | undefined;
    onChangeDelete: (config: LauncherConfig) => void;
    onClickDeleteConfig: () => void;
    onClickCloseDeleteDialog: () => void;
};

function ConfigDeleteDialog({
    deleteDialog,
    configToDelete,
    onChangeDelete,
    onClickDeleteConfig,
    onClickCloseDeleteDialog,
}: ConfigDeleteDialogProps) {
    const [configNameToDelete, setConfigNameToDelete] = useState('');

    const canConfirmDeletion = configNameToDelete === configToDelete?.name;

    useEffect(() => {
        if (!deleteDialog) {
            setConfigNameToDelete('');
        }
    }, [deleteDialog]);

    const onKeyDownConfigToDelete = useCallback(
        (event: React.KeyboardEvent) => {
            if (event.key === 'Enter' && canConfirmDeletion) {
                onClickDeleteConfig();
            }
        },
        [onClickDeleteConfig, canConfirmDeletion],
    );

    const onChangeConfigToDelete = useCallback(({ target }: any) => {
        setConfigNameToDelete(target.value as string);
    }, []);

    return (
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
    );
}

export default ConfigDeleteDialog;
