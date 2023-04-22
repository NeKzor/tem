import { useCallback, useContext } from 'react';
import { Select, Option } from '@material-tailwind/react';
import AppState, { DispatchAction } from '../AppState';

function Settings() {
    const {
        state: { checkForUpdates },
        dispatch,
    } = useContext(AppState);

    const onChangeCheckForUpdates = useCallback(
        (value: any) => {
            dispatch(DispatchAction.SetCheckForUpdates(value));
            dispatch(DispatchAction.SaveConfig());
        },
        [dispatch],
    );

    return (
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
    );
}

export default Settings;
