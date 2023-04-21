import { Typography } from '@material-tailwind/react';

function Footer() {
    return (
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
    );
}

export default Footer;
